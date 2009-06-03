/*
 * sipim.h
 *
 * Support for SIP session mode IM
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2008 Post Increment
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

#ifndef OPAL_IM_SIPIM_H
#define OPAL_IM_SIPIM_H

#include <ptlib.h>
#include <opal/buildopts.h>
#include <opal/rtpconn.h>
#include <opal/manager.h>
#include <opal/mediastrm.h>
#include <opal/mediatype.h>
#include <im/im.h>
#include <sip/sdp.h>


#if OPAL_HAS_SIPIM

class OpalSIPIMMediaType : public OpalIMMediaType 
{
  public:
    OpalSIPIMMediaType();
    virtual OpalMediaSession * CreateMediaSession(OpalConnection & conn, unsigned sessionID) const;

    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress);
};

////////////////////////////////////////////////////////////////////////////

/** Class for carrying MSRP session information
  */
class OpalSIPIMMediaSession : public OpalMediaSession
{
  PCLASSINFO(OpalSIPIMMediaSession, OpalMediaSession);
  public:
    OpalSIPIMMediaSession(OpalConnection & connection, unsigned sessionId);
    OpalSIPIMMediaSession(const OpalSIPIMMediaSession & _obj);

    virtual bool Open() { return true; }

    virtual void Close() { }

    virtual PObject * Clone() const { return new OpalSIPIMMediaSession(*this); }

    virtual bool IsActive() const { return true; }

    virtual bool IsRTP() const { return false; }

    virtual bool HasFailed() const { return false; }

    virtual OpalTransportAddress GetLocalMediaAddress() const;

    virtual void SetRemoteMediaAddress(const OpalTransportAddress &, const OpalMediaFormatList & );

    virtual SDPMediaDescription * CreateSDPMediaDescription(
      const OpalTransportAddress & localAddress
    );

    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat, 
      unsigned sessionID, 
      PBoolean isSource
    );

    virtual bool SendMessage(const PString & contentType, const PString & body);

    virtual bool SendIM(const PString & contentType, const PString & body);

    virtual PString GetCallID() const { return callId; }

  protected:
    OpalTransportAddress transportAddress;
    PString localURL;
    PString remoteURL;
    PString callId;
};

////////////////////////////////////////////////////////////////////////////

class OpalSIPIMMediaStream : public OpalIMMediaStream
{
  public:
    OpalSIPIMMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource,                       ///<  Is a source stream
      OpalSIPIMMediaSession & imSession
    );

    ~OpalSIPIMMediaStream();

    virtual bool Open();

    /**Read raw media data from the source media stream.
       The default behaviour reads from the PChannel object.
      */
    virtual PBoolean ReadData(
      BYTE * data,      ///<  Data buffer to read to
      PINDEX size,      ///<  Size of buffer
      PINDEX & length   ///<  Length of data actually read
    );

    /**Write raw media data to the sink media stream.
       The default behaviour writes to the PChannel object.
      */
    virtual PBoolean WriteData(
      const BYTE * data,   ///<  Data to write
      PINDEX length,       ///<  Length of data to read.
      PINDEX & written     ///<  Length of data actually written
    );

    /**Close the media stream.

       Closes the associated PChannel.
      */
    virtual PBoolean Close();
  //@}

  protected:
    OpalSIPIMMediaSession & m_imSession;
};

////////////////////////////////////////////////////////////////////////////

class SIPEndPoint;
class SIP_PDU;
class SIPConnection;

class OpalSIPIMManager : public PObject
{
  public:
    OpalSIPIMManager(SIPEndPoint & endpoint);
    void OnReceivedMessage(const SIP_PDU & pdu);

    bool StartSession(OpalSIPIMMediaSession * mediaSession);

    bool EndSession(OpalSIPIMMediaSession * mediaSession);

  protected:
    SIPEndPoint & m_endpoint;
    PMutex m_mutex;

    typedef std::map<std::string, OpalSIPIMMediaSession *> IMSessionMapType;
    IMSessionMapType m_imSessionMap;
};



////////////////////////////////////////////////////////////////////////////

#endif // OPAL_HAS_SIPIM

#endif // OPAL_IM_SIPIM_H
