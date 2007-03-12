/*
 * rfc2833.h
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
 * $Log: rfc2833.h,v $
 * Revision 1.2007  2007/03/12 23:36:23  csoutheren
 * Add support for Cisco NSE
 *
 * Revision 2.5  2005/11/30 13:35:26  csoutheren
 * Changed tags for Doxygen
 *
 * Revision 2.4  2002/09/16 02:52:33  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.3  2002/09/04 06:01:46  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.2  2002/02/19 07:34:44  robertj
 * Added OpalRFC2833 as a OpalMediaFormat variable.
 *
 * Revision 2.1  2002/01/22 05:35:28  robertj
 * Added RFC2833 support
 *
 */

#ifndef __OPAL_RFC2833_H
#define __OPAL_RFC2833_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <rtp/rtp.h>

extern char * OpalDefaultNTEString;

#if OPAL_T38FAX
extern char * OpalDefaultNSEString;
#endif

///////////////////////////////////////////////////////////////////////////////

class OpalRFC2833Info : public PObject {
    PCLASSINFO(OpalRFC2833Info, PObject);
  public:
    OpalRFC2833Info(
      char tone,
      unsigned duration = 0,
      unsigned timestamp = 0
    );

    char GetTone() const { return tone; }
    unsigned GetDuration() const { return duration; }
    unsigned GetTimestamp() const { return timestamp; }
    BOOL IsToneStart() const { return duration == 0; }

  protected:
    char     tone;
    unsigned duration;
    unsigned timestamp;
};


class OpalRFC2833Proto : public PObject {
    PCLASSINFO(OpalRFC2833Proto, PObject);
  public:
    OpalRFC2833Proto(
      const PNotifier & receiveNotifier
    );

    virtual BOOL SendTone(
      char tone,              ///<  DTMF tone code
      unsigned duration       ///<  Duration of tone in milliseconds
    );

    virtual BOOL BeginTransmit(
      char tone  ///<  DTMF tone code
    );
    virtual BOOL EndTransmit();

    virtual void OnStartReceive(
      char tone
    );
    virtual void OnEndReceive(
      char tone,
      unsigned duration,
      unsigned timestamp
    );

    RTP_DataFrame::PayloadTypes GetPayloadType() const { return payloadType; }

    void SetPayloadType(
      RTP_DataFrame::PayloadTypes type ///<  new payload type
    ) { payloadType = type; }

    const PNotifier & GetReceiveHandler() const { return receiveHandler; }
    const PNotifier & GetTransmitHandler() const { return transmitHandler; }

  protected:
    PDECLARE_NOTIFIER(RTP_DataFrame, OpalRFC2833Proto, ReceivedPacket);
    PDECLARE_NOTIFIER(RTP_DataFrame, OpalRFC2833Proto, TransmitPacket);
    PDECLARE_NOTIFIER(PTimer, OpalRFC2833Proto, ReceiveTimeout);
    PDECLARE_NOTIFIER(PTimer, OpalRFC2833Proto, TransmitEnded);

    RTP_DataFrame::PayloadTypes payloadType;

    PMutex mutex;

    PNotifier receiveNotifier;
    BOOL      receiveComplete;
    BYTE      receivedTone;
    unsigned  receivedDuration;
    unsigned  receiveTimestamp;
    PTimer    receiveTimer;
    PNotifier receiveHandler;

    enum {
      TransmitIdle,
      TransmitActive,
      TransmitEnding
    }         transmitState;
    BYTE      transmitCode;
    unsigned  transmitTimestamp;
    PTimer    transmitTimer;
    PNotifier transmitHandler;
};


#endif // __OPAL_RFC2833_H


/////////////////////////////////////////////////////////////////////////////
