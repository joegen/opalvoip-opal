/*
 * mediacmd.h
 *
 * Abstractions for sending commands to media processors.
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
 */

#ifndef OPAL_OPAL_MEDIACMD_H
#define OPAL_OPAL_MEDIACMD_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

///////////////////////////////////////////////////////////////////////////////

/** This is the base class for a command to a media transcoder and/or media
    stream. The commands are highly context sensitive, for example
    OpalVideoUpdatePicture would only apply to a video transcoder.
  */
class OpalMediaCommand : public PObject
{
  PCLASSINFO(OpalMediaCommand, PObject);
  public:
    OpalMediaCommand(
      const OpalMediaType & mediaType, ///< Media type to search for in open streams
      unsigned sessionID = 0,          ///< Session for media stream, 0 is use first \p mediaType stream
      unsigned ssrc = 0                ///< Sync Source for media stream (if RTP)
    );

  /**@name Overrides from PObject */
  //@{
    /**Standard stream print function.
       The PObject class has a << operator defined that calls this function
       polymorphically.
      */
    virtual void PrintOn(
      ostream & strm    ///<  Stream to output text representation
    ) const;

    /** Compare the two objects and return their relative rank. This function is
       usually overridden by descendent classes to yield the ranking according
       to the semantics of the object.
       
       The default function is to use the <code>CompareObjectMemoryDirect()</code>
       function to do a byte wise memory comparison of the two objects.

       @return
       <code>LessThan</code>, <code>EqualTo</code> or <code>GreaterThan</code>
       according to the relative rank of the objects.
     */
    virtual Comparison Compare(
      const PObject & obj   ///<  Object to compare against.
    ) const;

    // Redefine as pure, derived classes from here MUST implment it.
    virtual PObject * Clone() const = 0;
  //@}

  /**@name Operations */
  //@{
    /**Get the name of the command.
      */
    virtual PString GetName() const = 0;

    /**Get data buffer pointer for transfer to/from codec plug-in.
      */
    virtual void * GetPlugInData() const;

    /**Get data buffer size for transfer to/from codec plug-in.
      */
    virtual unsigned * GetPlugInSize() const;
  //@}

  /**@name Memebers */
  //@{
    const OpalMediaType & GetMediaType() const { return m_mediaType; }
    unsigned GetSessionID() const { return m_sessionID; }
    unsigned GetSyncSource() const { return m_ssrc; }
  //@}

  protected:
    OpalMediaType m_mediaType;
    unsigned      m_sessionID;
    unsigned      m_ssrc;
};


#define OPAL_DEFINE_MEDIA_COMMAND(cls, name, mediaType) \
  class cls : public OpalMediaCommand \
  { \
      PCLASSINFO_WITH_CLONE(cls, OpalMediaCommand) \
    public: \
      cls(unsigned id = 0, unsigned ssrc = 0) : OpalMediaCommand(mediaType, id, ssrc) { } \
      virtual PString GetName() const { return name; } \
  }


/**This indicates that the media flow (bit rate) is to be adjusted.
  */
class OpalMediaFlowControl : public OpalMediaCommand
{
    PCLASSINFO_WITH_CLONE(OpalMediaFlowControl, OpalMediaCommand);
  public:
    OpalMediaFlowControl(
      OpalBandwidth bitRate,           ///< Bandwidth to use
      const OpalMediaType & mediaType, ///< Media type to search for in open streams
      unsigned sessionID = 0,          ///< Session for media stream, 0 is use first \p mediaType stream
      unsigned ssrc = 0                ///< Sync Source for media stream (if RTP)
    );
    OpalMediaFlowControl(
      OpalBandwidth bitRate,           ///< Bandwidth to use
      const OpalMediaType & mediaType, ///< Media type to search for in open streams
      unsigned sessionID,              ///< Session for media stream, 0 is use first \p mediaType stream
      const RTP_SyncSourceArray & ssrcs ///< Sync Source(s) for media stream(s) (if RTP)
    );

    virtual PString GetName() const;

    const OpalBandwidth & GetMaxBitRate() const { return m_bitRate; }
    const RTP_SyncSourceArray & GetSSRCs() const { return m_ssrcs; }

  protected:
    OpalBandwidth m_bitRate;
    RTP_SyncSourceArray m_ssrcs;
};


/**This indicates the packet loss as percentage over the last arbitrary period.
  */
class OpalMediaPacketLoss : public OpalMediaCommand
{
    PCLASSINFO_WITH_CLONE(OpalMediaPacketLoss, OpalMediaCommand);
  public:
    OpalMediaPacketLoss(
      unsigned packetLoss,             ///< Packet loss as percentage
      const OpalMediaType & mediaType, ///< Media type to search for in open streams
      unsigned sessionID = 0,          ///< Session for media stream, 0 is use first \p mediaType stream
      unsigned ssrc = 0                ///< Sync Source for media stream (if RTP)
    );

    virtual PString GetName() const;

    unsigned GetPacketLoss() const { return m_packetLoss; }

  protected:
    unsigned m_packetLoss;
};


/**This indicates a new max payload size should be sued.
  */
class OpalMediaMaxPayload : public OpalMediaCommand
{
    PCLASSINFO_WITH_CLONE(OpalMediaMaxPayload, OpalMediaCommand);
  public:
    OpalMediaMaxPayload(
      unsigned payloadSize,            ///< New payload size
      const OpalMediaType & mediaType, ///< Media type to search for in open streams
      unsigned sessionID = 0,          ///< Session for media stream, 0 is use first \p mediaType stream
      unsigned ssrc = 0                ///< Sync Source for media stream (if RTP)
    );

    virtual PString GetName() const;

    unsigned GetPayloadSize() const { return m_payloadSize; }

  protected:
    unsigned m_payloadSize;
};


#endif // OPAL_OPAL_MEDIACMD_H


// End of File ///////////////////////////////////////////////////////////////
