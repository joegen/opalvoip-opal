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
 * Contributor(s): ______________________________________.
 *
 * $Log: main.h,v $
 * Revision 1.1  2006/06/19 09:32:09  dereksmithies
 * Initial cut of a program to test the jitter buffer in OPAL.
 *
 *
 */

#ifndef _Jester_MAIN_H
#define _Jester_MAIN_H


#include <rtp/jitter.h>
#include <rtp/rtp.h>
#include <ptclib/delaychan.h>
#include <ptclib/random.h>

/**Simulate an RTP_Session - this class will make up data received from the
   network */
class JestRTP_Session: public RTP_Session
{
    PCLASSINFO(JestRTP_Session, RTP_Session);

    /**Constructor, which just uses "any old random value" */
    JestRTP_Session();

  /**Read a data frame from the RTP channel.
       Any control frames received are dispatched to callbacks and are not
       returned by this function. It will block until a data frame is
       available or an error occurs.
      */
    virtual BOOL ReadData(
      RTP_DataFrame & frame   ///<  Frame read from the RTP session
	);

   /**Write a data frame from the RTP channel.
      */
    virtual BOOL WriteData(
      RTP_DataFrame & frame   ///<  Frame to write to the RTP session
	);

    /**Write a control frame from the RTP channel.
      */
    virtual BOOL WriteControl(
      RTP_ControlFrame & frame    ///<  Frame to write to the RTP session
	);

    /**Write the RTCP reports.
      */
    virtual BOOL SendReport() { PTRACE(4, "Send report has been quashed"); return TRUE; }

    /**Close down the RTP session.
      */
    virtual void Close(
	BOOL /*reading */   ///<  Closing the read side of the session
	);

   /**Reopens an existing session in the given direction.
      */
    virtual void Reopen(
	BOOL /*isReading */
	) { }

    /**Get the local host name as used in SDES packes.
      */
    virtual PString GetLocalHostName() { return PString("Jester"); }



 protected:
    /**psuedo sequence number that we will put into the packets */
    WORD psuedoSequenceNo;

    /**psuedo timestamp that we will put into the packets. Timestamp goes up
     * by the number of samples placed in the packet. So, 8khz, 30ms duration,
     * means 240 increment for each packet. */
    DWORD psuedoTimestamp;

    /**Flag to indicate we have closed down */
    BOOL closedDown;

    /**time at which this all started */
    PTime startJester;

    /**Number of times we have read a packet. This is required to determine the required time period
       to sleep */
    PINDEX readCount;

    /**Seed for a random number generator */
    PRandom variation;
};


class JesterProcess : public PProcess
{
  PCLASSINFO(JesterProcess, PProcess)

  public:
    JesterProcess();

    void Main();

  protected:

};


#endif  // _Jester_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
