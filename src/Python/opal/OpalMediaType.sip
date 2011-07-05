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
#include <opal/mediatype.h>
%End


/** Define the type used to hold the media type identifiers, i.e. "audio", "video", "h.224", "fax" etc
  */
class OpalMediaType
{

  public:
    OpalMediaType();

    virtual ~OpalMediaType();

	OpalMediaType(const std::string & str);
    OpalMediaType(const char * str);
    OpalMediaType(const PString & str);

    static const OpalMediaType & Audio();
    static const OpalMediaType & Video();
    static const OpalMediaType & Fax();
    static const OpalMediaType & UserInput();

    OpalMediaTypeDefinition * GetDefinition() const;
    static OpalMediaTypeDefinition * GetDefinition(const OpalMediaType & key);
    static OpalMediaTypeDefinition * GetDefinition(unsigned sessionId /Constrained/);

    
    static OpalMediaType GetMediaTypeFromSDP(const std::string & key, const std::string & transport);


    enum AutoStartMode {
      // Do not change order of enum as useful for bitmasking rx/tx
      OfferInactive,
      Receive,
      Transmit,
      ReceiveTransmit,
      DontOffer,

      TransmitReceive = ReceiveTransmit
    };

    AutoStartMode GetAutoStart() const;
	
	SIP_PYOBJECT __repr__();
%MethodCode
	sipRes = PyString_FromString(sipCpp->c_str());
%End

	//allow python to compare media types
	bool __eq__(const OpalMediaType &) const;
%MethodCode
	sipRes = (*sipCpp == *a0);
%End

	bool __ne__(const OpalMediaType &) const;
%MethodCode
	sipRes = (*sipCpp != *a0);
%End

};



/** This class defines the type used to define the attributes of a media type
 */
class OpalMediaTypeDefinition /Abstract/
{
  public:
    /// Create a new media type definition
    OpalMediaTypeDefinition(
      const char * mediaType,          ///< name of the media type (audio, video etc)
      const char * sdpType,            ///< name of the SDP type 
      unsigned requiredSessionId = 0,  ///< Session ID to use, asserts if already in use
      OpalMediaType::AutoStartMode autoStart = OpalMediaType::DontOffer   ///< Default value for auto-start transmit & receive
    );

    // Needed to avoid gcc warning about classes with virtual functions and 
    //  without a virtual destructor
    virtual ~OpalMediaTypeDefinition();

    /** Get flags for media type can auto-start on call initiation.
      */
    OpalMediaType::AutoStartMode GetAutoStart() const;

    /** Set flag for media type can auto-start receive on call initiation.
      */
    void SetAutoStart(OpalMediaType::AutoStartMode v);
    void SetAutoStart(OpalMediaType::AutoStartMode v, bool on);

    /** Indicate type uses RTP for transport.
        If false, then it uses a generic OpaMediaSession
      */
    virtual bool UsesRTP() const;

    /** Create a media session suitable for the media type.
      */
    virtual OpalMediaSession * CreateMediaSession(
      OpalConnection & connection,  ///< Connection media session is being created for
      unsigned         sessionID    ///< ID for the media session
    ) const;

    /** Get the string used for the RTP_FormatHandler PFactory which is used
        to create the RTP handler for the this media type
        possible values include "rtp/avp" and "udptl"

        Only valid if UsesRTP return true
      */
    virtual PString GetRTPEncoding() const = 0;
	
	/** Create an RTP session for this media format.
        By default, this will create a RTP_UDP session with the correct initial format

        Only valid if UsesRTP return true
      */
    virtual RTP_UDP * CreateRTPSession(
      OpalRTPConnection & conn,
      unsigned sessionID, 
      bool remoteIsNAT
    );

    /** Return the default session ID for this media type.
      */
    unsigned GetDefaultSessionId() const;

    //
    //  return the SDP type for this media type
    //
    virtual std::string GetSDPType() const;

    //
    //  create an SDP media description entry for this media type
    //
   virtual SDPMediaDescription * CreateSDPMediaDescription(
      const OpalTransportAddress & localAddress
    ) = 0;
};



////////////////////////////////////////////////////////////////////////////
//
//  common ancestor for audio and video OpalMediaTypeDefinitions
//

class OpalRTPAVPMediaType : OpalMediaTypeDefinition {
  public:
    OpalRTPAVPMediaType(
      const char * mediaType, 
      const char * sdpType, 
      unsigned     requiredSessionId = 0,
      OpalMediaType::AutoStartMode autoStart = OpalMediaType::DontOffer
    );

    virtual PString GetRTPEncoding() const;

    OpalMediaSession * CreateMediaSession(OpalConnection & /*conn*/, unsigned /* sessionID*/) const;
};


class OpalAudioMediaType : OpalRTPAVPMediaType {
  public:
    OpalAudioMediaType();

    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress);
};



class OpalVideoMediaType : OpalRTPAVPMediaType {
  public:
    OpalVideoMediaType();

    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress);
};



class OpalFaxMediaType : OpalMediaTypeDefinition 
{
  public:
    OpalFaxMediaType();

    PString GetRTPEncoding() const;
    RTP_UDP * CreateRTPSession(OpalRTPConnection & conn,
                               unsigned sessionID, bool remoteIsNAT);

    OpalMediaSession * CreateMediaSession(OpalConnection & conn, unsigned /* sessionID*/) const;

    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress);
};
