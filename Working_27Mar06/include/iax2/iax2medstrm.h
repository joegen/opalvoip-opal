/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Open Phone Abstraction Library (OPAL)
 *
 * Extension of the Opal Media stream, where the media from the IAX2 side is
 * linked to the OPAL 
 *
 * Copyright (c) 2005 Indranet Technologies Ltd.
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
 * The Initial Developer of the Original Code is Indranet Technologies Ltd.
 *
 * The author of this code is Derek J Smithies
 *
 * $Log: iax2medstrm.h,v $
 * Revision 1.1  2005/07/30 07:01:32  csoutheren
 * Added implementation of IAX2 (Inter Asterisk Exchange 2) protocol
 * Thanks to Derek Smithies of Indranet Technologies Ltd. for
 * writing and contributing this code
 *
 *
 *
 */

#ifndef __OPAL_IAX2_MEDIASTRM_H
#define __OPAL_IAX2_MEDIASTRM_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <opal/buildopts.h>
#include <opal/mediafmt.h>
#include <iax2/iax2con.h>

class RTP_Session;
class OpalMediaPatch;
class OpalLine;


/**This class describes a media stream, which is an interface to the opal classes for 
   generating encoded media data 
*/
class OpalIAX2MediaStream : public OpalMediaStream
{
  PCLASSINFO(OpalIAX2MediaStream, OpalMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for connecting to the media 
      */
    OpalIAX2MediaStream(
		   const OpalMediaFormat & mediaFormat, /*!< Media format for stream */
		   unsigned sessionID,                  /*!< Session number for stream */
		   BOOL isSource,                       /*!< Is a source stream */
		   unsigned minJitterDelay,             /*!< Minimum delay from jitter buffer (ms) */
		   unsigned maxJitterDelay,             /*!< Maximum delay from jitter buffer (ms) */
		   IAX2Connection &con                  /*!< IAX connection to read/send incoming packets */
		   );
  //@}
 
  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Open the media stream.
 
 
      */
    virtual BOOL Open();
 
    /**Start the media stream.
       
    The default behaviour calls Resume() on the associated
    OpalMediaPatch thread if it was suspended.
    */
    virtual BOOL Start();

    /**Close the media stream.
 
       The default does nothing.
      */
    virtual BOOL Close();
 
    /**Read raw media data from the source media stream.
       The default behaviour reads from the OpalLine object.
      */
    virtual BOOL ReadData(
      BYTE * data,      /// Data buffer to read to
      PINDEX size,      /// Size of buffer
      PINDEX & length   /// Length of data actually read
    );

   /**Write raw media data to the sink media stream.
       The default behaviour writes to the OpalLine object.
      */
    virtual BOOL WriteData(
      const BYTE * data,   /// Data to write
      PINDEX length,       /// Length of data to read.
      PINDEX & written     /// Length of data actually written
    );

    /**Indicate if the media stream is synchronous.
       A synchronous stream is one that is regular, such as the sound frames
       from a sound card.
      */
    virtual BOOL IsSynchronous() const;
  //@}

  protected:
    /**The connection is the source/sink of our data packets */
    IAX2Connection &connection;

    /**There was unused data from an incoming ethernet frame. The
       unused data is stored here. 
    */
    PBYTEArray pendingData;

    /**The endpoint has specified the min delay from a jitter buffer*/
    unsigned      minAudioJitterDelay;

    /**The endpoint has specified the max delay from a jitter buffer*/
    unsigned      maxAudioJitterDelay;

};

#endif  //__OPAL_IAXs_MEDIASTRM_H

/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 2 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-file-style:linux
 * c-basic-offset:2
 * End:
 */

