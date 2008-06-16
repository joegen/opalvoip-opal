/*
 * opal.h
 *
 * "C" language interface for OPAL
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2008 Vox Lucida
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
 * The Initial Developer of the Original Code is Vox Lucida (Robert Jongbloed)
 *
 * This code was initially written with the assisance of funding from
 * Stonevoice. http://www.stonevoice.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef __OPAL_H
#define __OPAL_H

#ifdef __cplusplus
extern "C" {
#endif

/** /file opal.h
    This file contains an API for accessing the OPAL system via "C" language interface.
    A contrained set of functions are declared which allows the library to be easily
    "late bound" using Windows LoadLibrary() or Unix dlopen() at run time.
  */

#ifdef _WIN32
  #define OPAL_EXPORT __stdcall
#else
  #define OPAL_EXPORT
#endif

typedef struct OpalHandleStruct * OpalHandle;

typedef struct OpalMessage OpalMessage;


#define OPAL_C_API_VERSION 5


///////////////////////////////////////

/** Initialise the OPAL system, returning a "handle" to the system that must
    be used in other calls to OPAL.

    The version parameter indicates the version of the API being used by the
    caller. It should always be set to the constant OPAL_C_API_VERSION. On
    return the library will indicate the API version it supports, if it is
    lower than that provided by the application.

    The C string options are space separated tokens indicating various options
    to be enabled, for example the protocols to be available. NULL or an empty
    string will load all available protocols. The current protocol tokens are:

        pc sip sips h323 h323s iax2 pots pstn ivr

    Additional options are:

        TraceLevel=1     Level for tracing. 
        TraceFile="name" Set the filename for trace output. Note quotes are
                         required if spaces are in filename.
    It should also be noted that there must not be spaces around the '=' sign
    in the above options.

    If NULL is returned then an initialisation error occurred. This can only
    really occur if the user specifies prefixes which are not supported by
    the library.
  */
OpalHandle OPAL_EXPORT OpalInitialise(unsigned * version, const char * options);

/** String representation of the OpalIntialise() which may be used for late
    binding to the library.
 */
#define OPAL_INITIALISE_FUNCTION   "OpalInitialise"

/** Typedef representation of the pointer to the OpalIntialise() function which
    may be used for late binding to the library.
 */
typedef OpalHandle (OPAL_EXPORT *OpalInitialiseFunction)(unsigned * version, const char * options);


///////////////////////////////////////

/** Shut down and clean up all resource used by the OPAL system. The parameter
    must be the handle returned by OpalInitialise().
  */
void OPAL_EXPORT OpalShutDown(OpalHandle opal);

/** String representation of the OpalShutDown() which may be used for late
    binding to the library.
 */
#define OPAL_SHUTDOWN_FUNCTION     "OpalShutDown"

/** Typedef representation of the pointer to the OpalShutDown() function which
    may be used for late binding to the library.
 */
typedef void (OPAL_EXPORT *OpalShutDownFunction)(OpalHandle opal);


///////////////////////////////////////

/** Get a message from the OPAL system. The first parameter must be the handle
    returned by OpalInitialise(). The second parameter is a timeout in
    milliseconds. NULL is returned if a timeout occurs. A value of UINT_MAX
    will wait forever for a message.

    The returned message must be disposed of by a call to OpalFreeMessage().

    Note if OpalShutDown() is called from a different thread then this function
    will break from its block and return NULL.
  */
OpalMessage * OPAL_EXPORT OpalGetMessage(OpalHandle opal, unsigned timeout);

/** String representation of the OpalGetMessage() which may be used for late
    binding to the library.
 */
#define OPAL_GET_MESSAGE_FUNCTION  "OpalGetMessage"

/** Typedef representation of the pointer to the OpalGetMessage() function which
    may be used for late binding to the library.
 */
typedef OpalMessage * (OPAL_EXPORT *OpalGetMessageFunction)(OpalHandle opal, unsigned timeout);


///////////////////////////////////////

/** Send a message to the OPAL system. The first parameter must be the handle
    returned by OpalInitialise(). The second parameter is a constructed message
    which is a command to the OPAL system.

    Within the command message, generally a NULL or empty string, or zero value
    for integral types, indicates the particular parameter is to be ignored.
    Documentation on individiual messages will indicate which are mandatory.
    
    The return value is another message which will have a type of
    OpalIndCommandError if an error occurs. The OpalMessage::m_commandError field
    will contain a string indicating the error that occurred.

    If successful, the the type of the message is the same as the command type.
    The message fields in the return will generally be set to the previous value
    for the field, where relevant. For example in the OpalCmdSetGeneralParameters
    command the OpalParamGeneral::m_stunServer would contain the STUN server name
    prior to the command.

    A NULL is only returned if the either OpalHandle or OpalMessage parameters is NULL.

    The returned message must be disposed of by a call to OpalFreeMessage().
  */
OpalMessage * OPAL_EXPORT OpalSendMessage(OpalHandle opal, const OpalMessage * message);

/** String representation of the OpalSendMessage() which may be used for late
    binding to the library.
 */
typedef OpalMessage * (OPAL_EXPORT *OpalSendMessageFunction)(OpalHandle opal, const OpalMessage * message);

/** Typedef representation of the pointer to the OpalSendMessage() function which
    may be used for late binding to the library.
 */
#define OPAL_SEND_MESSAGE_FUNCTION "OpalSendMessage"


///////////////////////////////////////

/** Free memeory in message the OPAL system has sent. The parameter must be
    the message returned by OpalGetMessage() or OpalSendMessage().
  */
void OPAL_EXPORT OpalFreeMessage(OpalMessage * message);

/** String representation of the OpalFreeMessage() which may be used for late
    binding to the library.
 */
#define OPAL_FREE_MESSAGE_FUNCTION "OpalFreeMessage"

/** Typedef representation of the pointer to the OpalFreeMessage() function which
    may be used for late binding to the library.
 */
typedef void (OPAL_EXPORT *OpalFreeMessageFunction)(OpalMessage * message);


///////////////////////////////////////

#define OPAL_PREFIX_PCSS  "pc"
#define OPAL_PREFIX_LOCAL "local"
#define OPAL_PREFIX_H323  "h323"
#define OPAL_PREFIX_SIP   "sip"
#define OPAL_PREFIX_IAX2  "iax2"
#define OPAL_PREFIX_POTS  "pots"
#define OPAL_PREFIX_PSTN  "pstn"
#define OPAL_PREFIX_IVR   "ivr"

#define OPAL_PREFIX_ALL OPAL_PREFIX_PCSS  " " \
                        OPAL_PREFIX_LOCAL " " \
                        OPAL_PREFIX_H323  " " \
                        OPAL_PREFIX_SIP   " " \
                        OPAL_PREFIX_IAX2  " " \
                        OPAL_PREFIX_POTS  " " \
                        OPAL_PREFIX_PSTN  " " \
                        OPAL_PREFIX_IVR


/**Type code the silence detect algorithm modes.
   This is used by the OpalCmdSetGeneralParameters command in the OpalParamGeneral structure.
  */
typedef enum OpalSilenceDetectModes {
  OpalSilenceDetectNoChange,  /**< No change to the silence detect mode. */
  OpalSilenceDetectDisabled,  /**< Indicate silence detect is disabled */
  OpalSilenceDetectFixed,     /**< Indicate silence detect uses a fixed threshold */
  OpalSilenceDetectAdaptive   /**< Indicate silence detect uses an adaptive threashold */
} OpalSilenceDetectModes;


/**Type code the echo cancellation algorithm modes.
   This is used by the OpalCmdSetGeneralParameters command in the OpalParamGeneral structure.
  */
typedef enum OpalEchoCancelMode {
  OpalEchoCancelNoChange,   /**< No change to the echo cancellation mode. */
  OpalEchoCancelDisabled,   /**< Indicate the echo cancellation is disabled */
  OpalEchoCancelEnabled     /**< Indicate the echo cancellation is enabled */
} OpalEchoCancelMode;


/** Function for reading/writing media data.
    Returns size of data actually read or written, or -1 if there is an error
    and the media stream should be shut down.
 */
typedef int (*OpalMediaDataFunction)(
  const char * token,   /**< Call token for media data as returned by OpalIndIncomingCall.
                             This may be used to discriminate between individiual calls. */
  const char * stream,  /**< Stream identifier for media data. This may be used to
                             discriminate between media streams within a call, applicable
                             if there can be more than one stream of a particular format,
                             e.g. two H.263 video channels. */
  const char * format,  /**< Format of media data, e.g. "PCM-16" */
  void * data,          /**< Data to read/write */
  int size              /**< Maximum size of data to read, or size of actual data to write */
);


/**Type code the media data call back functions data type.
   This is used by the OpalCmdSetGeneralParameters command in the OpalParamGeneral structure.
  */
typedef enum OpalMediaDataType {
  OpalMediaDataNoChange,      /**< No change to the media data type. */
  OpalMediaDataPayloadOnly,   /**< Indicate only the RTP payload is passed to the
                                   read/write function */
  OpalMediaDataWithHeader     /**< Indicate the while RTP frame including header is
                                   passed to the read/write function */
} OpalMediaDataType;


/**Type code for messages defined by OpalMessage.
  */
typedef enum OpalMessageType {
  OpalIndCommandError,          /**<An error occurred during a command. This is only returned by
                                    OpalSendMessage(). The details of the error are shown in the
                                    OpalMessage::m_commandError field. */
  OpalCmdSetGeneralParameters,  /**<Set general parameters command. This configures global settings in OPAL.
                                    See the OpalParamGeneral structure for more information. */
  OpalCmdSetProtocolParameters, /**<Set protocol parameters command. This configures settings in OPAL that
                                    may be different for each protocol, e.g. SIP & H.323. See the 
                                    OpalParamProtocol structure for more information. */
  OpalCmdRegistration,          /**<Register/Unregister command. This initiates a registration or
                                    unregistration operation with a protocol dependent server. Currently
                                    only for H.323 and SIP. See the OpalParamRegistration structure for more
                                    information. */
  OpalIndRegistration,          /**<Status of registration indication. After the OpalCmdRegistration has
                                    initiated a registration, this indication will be returned by the
                                    OpalGetMessage() function when the status of the registration changes,
                                    e.g. successful registration or communications failure etc. See the
                                    OpalStatusRegistration structure for more information. */
  OpalCmdSetUpCall,             /**<Set up a call command. This starts the outgoing call process. The
                                    OpalIndAlerting, OpalIndEstablished and OpalIndCallCleared messages are
                                    returned by OpalGetMessage() to indicate the call progress. See the 
                                    OpalParamSetUpCall structure for more information. */
  OpalIndIncomingCall,          /**<Incoming call indication. This is returned by the OpalGetMessage() function
                                    at any time after listeners are set up via the OpalCmdSetProtocolParameters
                                    command. See the OpalStatusIncomingCall structure for more information. */
  OpalCmdAnswerCall,            /**<Answer call command. After a OpalIndIncomingCall is returned by the
                                    OpalGetMessage() function, an application maye indicate that the call is
                                    to be answered with this message. The OpalMessage m_callToken field is
                                    set to the token returned in OpalIndIncomingCall. */
  OpalCmdClearCall,             /**<Hang Up call command. After a OpalCmdSetUpCall command is executed or a
                                    OpalIndIncomingCall indication is received then this may be used to
                                    "hang up" the call. The OpalMessage m_callToken field is set to the
                                    token returned in OpalCmdSetUpCall or OpalIndIncomingCall. The
                                    OpalIndCallCleared is returned in the OpalGetMessage() when the call has
                                    completed its hang up operation. */
  OpalIndAlerting,              /**<Remote is alerting indication. This message is returned in the
                                    OpalGetMessage() function when the underlying protocol states the remote
                                    telephone is "ringing". See the  OpalParamSetUpCall structure for more
                                    information. */
  OpalIndMediaStream,           /**<A media stream has started/stopped. This message is returned in the
                                    OpalGetMessage() function when a media stream is started or stopped. See the
                                    OpalStatusMediaStream structure for more information. */
  OpalIndEstablished,           /**<Call is established indication. This message is returned in the
                                    OpalGetMessage() function when the remote or local endpont has "answered"
                                    the call and there is media flowing. See the  OpalParamSetUpCall
                                    structure for more information. */
  OpalIndUserInput,             /**<User input indication. This message is returned in the OpalGetMessage()
                                    function when, during a call, user indications (aka DTMF tones) are
                                    received. See the OpalStatusUserInput structure for more information. */
  OpalIndCallCleared,           /**<Call is cleared indication. This message is returned in the
                                    OpalGetMessage() function when the call has completed. The OpalMessage
                                    m_callToken field indicates which call cleared. */
  OpalCmdHoldCall,              /**<Place call in a hold state. The OpalMessage m_callToken field is set to
                                    the token returned in OpalIndIncomingCall. */
  OpalCmdRetrieveCall,          /**<Retrieve call from hold state. The OpalMessage m_callToken field is set
                                    to the token returned in OpalIndIncomingCall. */
  OpalCmdTransferCall,          /**<Transfer a call to another party. This starts the outgoing call process
                                    for the other party. See the  OpalParamSetUpCall structure for more
                                    information.*/
  OpalCmdUserInput,             /**<User input command. This sends specified user input to the remote
                                    connection. See the OpalStatusUserInput structure for more information. */
  OpalIndMessageWaiting,        /**<Message Waiting indication. This message is returned in the
                                    OpalGetMessage() function when an MWI is received on any of the supported
                                    protocols.
                                */
  OpalMessageTypeCount
} OpalMessageType;


/**General parameters for the OpalCmdSetGeneralParameters command.
   This is only passed to and returned from the OpalSendMessage() function.
  */
typedef struct OpalParamGeneral {
  const char * m_audioRecordDevice;   /**< Audio recording device name */
  const char * m_audioPlayerDevice;   /**< Audio playback device name */
  const char * m_videoInputDevice;    /**< Video input (e.g. camera) device name */
  const char * m_videoOutputDevice;   /**< Video output (e.g. window) device name */
  const char * m_videoPreviewDevice;  /**< Video preview (e.g. window) device name */
  const char * m_mediaOrder;          /**< List of media format names to set the preference order for media.
                                           This list of names (e.g. "G.723.1") is separated by the '\n'
                                           character. */
  const char * m_mediaMask;           /**< List of media format names to set media to be excluded.
                                           This list of names (e.g. "G.723.1") is separated by the '\n'
                                           character. */
  const char * m_autoRxMedia;         /**< List of media types (e.g. audio, video) separated by spaces
                                           which may automatically be received automatically. */
  const char * m_autoTxMedia;         /**< List of media types (e.g. audio, video) separated by spaces
                                           which may automatically be transmitted automatically. */
  const char * m_natRouter;           /**< The host name or IP address of the Network Address Translation
                                           router which may be between the endpoint and the Internet. */
  const char * m_stunServer;          /**< The host name or IP address of the STUN server which may be
                                           used to determine the NAT router characteristics automatically. */
  unsigned     m_tcpPortBase;         /**< Base of range of ports to use for TCP communications. This may
                                           be required by some firewalls. */
  unsigned     m_tcpPortMax;          /**< Max of range of ports to use for TCP communications. This may
                                           be required by some firewalls. */
  unsigned     m_udpPortBase;         /**< Base of range of ports to use for UDP communications. This may
                                           be required by some firewalls. */
  unsigned     m_udpPortMax;          /**< Max of range of ports to use for UDP communications. This may
                                           be required by some firewalls. */
  unsigned     m_rtpPortBase;         /**< Base of range of ports to use for RTP/UDP communications. This may
                                           be required by some firewalls. */
  unsigned     m_rtpPortMax;          /**< Max of range of ports to use for RTP/UDP communications. This may
                                           be required by some firewalls. */
  unsigned     m_rtpTypeOfService;    /**< Value for the Type Of Service byte with UDP/IP packets which may
                                           be used by some routers for simple Quality of Service control. */
  unsigned     m_rtpMaxPayloadSize;   /**< Maximum payload size for RTP packets. This may sometimes need to
                                           be set according to the MTU or the underlying network. */
  unsigned     m_minAudioJitter;      /**< Minimum jitter time in milliseconds. For audio RTP data being
                                           received this sets the minimum time of the adaptive jitter buffer
                                           which smooths out irregularities in the transmission of audio
                                           data over the Internet. */
  unsigned     m_maxAudioJitter;      /**< Maximum jitter time in milliseconds. For audio RTP data being
                                           received this sets the maximum time of the adaptive jitter buffer
                                           which smooths out irregularities in the transmission of audio
                                           data over the Internet. */
  OpalSilenceDetectModes m_silenceDetectMode; /**< Silence detection mode. This controls the silence
                                           detection algorithm for audio transmission: 0=no change,
                                           1=disabled, 2=fixed, 3=adaptive. */
  unsigned     m_silenceThreshold;    /**< Silence detection threshold value. This applies if
                                           m_silenceDetectMode is fixed (2) and is a PCM-16 value. */
  unsigned     m_signalDeadband;      /**< Time signal is required before audio is transmitted. This is
                                           is RTP timestamp units (8000Hz). */
  unsigned     m_silenceDeadband;     /**< Time silence is required before audio is transmission is stopped.
                                           This is is RTP timestamp units (8000Hz). */
  unsigned     m_silenceAdaptPeriod;  /**< Window for adapting the silence threashold. This applies if
                                           m_silenceDetectMode is adaptive (3). This is is RTP timestamp
                                           units (8000Hz). */
  OpalEchoCancelMode m_echoCancellation; /**< Accoustic Echo Cancellation control. 0=no change, 1=disabled,
                                              2=enabled. */
  unsigned     m_audioBuffers;        /**< Set the number of hardware sound buffers to use. */
  OpalMediaDataFunction m_mediaReadData;   /**< Callback function for reading raw media data. */
  OpalMediaDataFunction m_mediaWriteData;  /**< Callback function for writing raw media data. */
  OpalMediaDataType     m_mediaDataHeader; /**< Indicate that the media read/write callback function
                                           is passed the full RTP header or just the payload.
                                           0=no change, 1=payload only, 2=with RTP header. */
} OpalParamGeneral;


/**Protocol parameters for the OpalCmdSetProtocolParameters command.
   This is only passed to and returned from the OpalSendMessage() function.
  */
typedef struct OpalParamProtocol {
  const char * m_prefix;              /**< Protocol prefix for parameters, e.g. "h323" or "sip". If this is
                                           NULL or empty string, then the parameters are set for all protocols
                                           where they maybe set. */
  const char * m_userName;            /**< User name to identify the endpoint. This is usually the protocol
                                           specific name and may interact with the OpalCmdRegistration
                                           command. e.g. "robertj or 61295552148 */
  const char * m_displayName;         /**< Display name to be used. This is the human readable form of the
                                           users name, e.g. "Robert Jongbloed". */
  const char * m_vendor;              /**< Name of the vendor or manufacturer of the application. This is
                                           used to identify the software which can be very useful for
                                           solving interoperability issues. e.g. "Vox Lucida". */
  const char * m_name;                /**< Name of the product within the vendor name space. This is
                                           used to identify the software which can be very useful for
                                           solving interoperability issues. e.g. "OpenPhone". */
  const char * m_version;             /**< Version of the product within the vendor/product name space. This
                                           is used to identify the software which can be very useful for
                                           solving interoperability issues. e.g. "2.1.4". */
  unsigned     m_t35CountryCode;      /**< T.35 country code for the name space in which the vendor or
                                           manufacturer is identified. This is the part of the H.221
                                           equivalent of the m_vendor string above and  used to identify the
                                           software which can be very useful for solving interoperability
                                           issues. e.g. 9 is for Australia. */
  unsigned     m_t35Extension;        /**< T.35 country extension code for the name space in which the vendor or
                                           manufacturer is identified. This is the part of the H.221
                                           equivalent of the m_vendor string above and  used to identify the
                                           software which can be very useful for solving interoperability
                                           issues. Very rarely used. */
  unsigned     m_manufacturerCode;    /**< Manuacturer code for the name space in which the vendor or
                                           manufacturer is identified. This is the part of the H.221
                                           equivalent of the m_vendor string above and  used to identify the
                                           software which can be very useful for solving interoperability
                                           issues. e.g. 61 is for Equivalance and was allocated by the
                                           Australian Communications Authority, Oct 2000. */
  const char * m_interfaceAddresses;  /**< A list of interfaces to start listening for incoming calls.
                                           This list is separated by the '\n' character. If NULL no
                                           listeners are started or stopped. If and empty string ("")
                                           then all listeners are stopped. If a "*" then listeners
                                           are started for all interfaces in the system. */
} OpalParamProtocol;


/**Registration parameters for the OpalCmdRegistration command.
   This is only passed to and returned from the OpalSendMessage() function.
  */
typedef struct OpalParamRegistration {
  const char * m_protocol;      /**< Protocol prefix for registration. Currently must be "h323" or
                                     "sip", cannot be NULL. */
  const char * m_identifier;    /**< Identifier for name to be registered at server. If NULL
                                     or empty then the value provided in the OpalParamProtocol::m_userName
                                     field of the OpalCmdSetProtocolParameters command is used. Note
                                     that for SIP the value will have "@" and the
                                     OpalParamRegistration::m_hostName field apepnded to it to create
                                     and Address-Of_Record. */
  const char * m_hostName;      /**< Host or domain name for server. For SIP this cannot be NULL.
                                     For H.323 a NULL value indicates that a broadcast discovery is
                                     be performed. */
  const char * m_authUserName;  /**< User name for authentication. */
  const char * m_password;      ///< Password for authentication with server.
  const char * m_adminEntity;   /**< Identification of the administrative entity. For H.323 this will
                                     by the gatekeeper identifier. For SIP this is the authentication
                                     realm. */
  unsigned     m_timeToLive;    ///< Time between registration updates in secodns.
} OpalParamRegistration;


/**Registration status for the OpalIndRegistration indication.
   This is only returned from the OpalGetMessage() function.
  */
typedef struct OpalStatusRegistration {
  const char * m_protocol;    /**< Protocol prefix for registration. Currently must be "h323" or
                                   "sip", is never NULL. */
  const char * m_serverName;  /**< Nmae of the registration server. The exact format is protocol
                                   specific but generally contains the host or domain name, e.g.
                                   "GkId@gatekeeper.voxgratia.org" or "sip.voxgratia.org". */
  const char * m_error;       /**< Error message for registration. If any error in the initial
                                   registration or any subsequent registration update occurs, then
                                   this contains a string indicating the type of error. If no
                                   error occured then this will be NULL. */
} OpalStatusRegistration;


/**Set up call parameters for several command and indication messages.

   When establishing a new call via the OpalCmdSetUpCall command, the m_partyA and
   m_partyB fields indicate the parties to connect.

   For OpalCmdTransferCall, m_partyA indicates the connection to be transferred and
   m_partyB is the party to be transferred to. If the call transfer is successful then
   a OpalIndCallCleared message will be received clearing the local call.

   For OpalIndAlerting and OpalIndEstablished indications the three fields are set
   to the data for the call in progress.
  */
typedef struct OpalParamSetUpCall {
  const char * m_partyA;      /**< A-Party for call.

                                   For OpalCmdSetUpCall, this indicates what subsystem will
                                   be starting the call, e.g. "pots:Handset One". If NULL
                                   or empty string then "pc:*" is used indication that the
                                   standard PC sound system ans screen is to be used.

                                   For OpalCmdTransferCall this indicates the party to be
                                   transferred, e.g. "sip:fred@nurk.com". If NULL then
                                   it is assumed that the party that is not the pc or pots
                                   connection is to be transferred.

                                   For OpalIndAlerting and OpalIndEstablished this indicates
                                   the A-party of the call in progress. */
  const char * m_partyB;      /**< B-Party for call. This is typically a remote host URL
                                   address with protocol, e.g. "h323:simple.com" or
                                   "sip:fred@nurk.com".

                                   This must be provided in the OpalCmdSetUpCall and
                                   OpalCmdTransferCall commands, and is set by the system
                                   in the OpalIndAlerting and OpalIndEstablished indications.

                                   If used in the OpalCmdTransferCall command, this may be
                                   a valid call token for another call on hold. The remote
                                   is transferred to the call on hold and both calls are
                                   then cleared. */
  const char * m_callToken;   /**< Value of call token for new call. The user would pass NULL
                                   for this string in OpalCmdSetUpCall, a new value is
                                   returned by the OpalSendMessage() function. The user would
                                   provide the call token for the call being transferred when
                                   OpalCmdTransferCall is being called. */
} OpalParamSetUpCall;


/**Incoming call information for the OpalIndIncomingCall indication.
   This is only returned from the OpalGetMessage() function.
  */
typedef struct OpalStatusIncomingCall {
  const char * m_callToken;     ///< Call token for new call.
  const char * m_localAddress;  ///< Address of local interface. e.g. "sip:me@here.com"
  const char * m_remoteAddress; ///< Address of remote party. e.g. "sip:them@there.com"
} OpalStatusIncomingCall;


/**Media stream information for the OpalIndMediaStream indication.
   This is only returned from the OpalGetMessage() function.
  */
typedef struct OpalStatusMediaStream {
  const char * m_callToken;   ///< Call token for the call the media stream is.
  const char * m_identifier;  ///< Unique identifier for the media stream
  const char * m_format;      ///< Media format for the stream.
  unsigned     m_status;      ///< Status of stream, 1=started, 2=stopped
} OpalStatusMediaStream;


/**User input information for the OpalIndUserInput indication.
   This is only returned from the OpalGetMessage() function.
  */
typedef struct OpalStatusUserInput {
  const char * m_callToken;   ///< Call token for the call the User Input was received on.
  const char * m_userInput;   ///< User input string, e.g. "#".
  unsigned     m_duration;    /**< Duration in milliseconds for tone. For DTMF style user input
                                   the time the tone was detected may be placed in this field.
                                   Generally zero is passed which means the m_userInput is a
                                   single "string" input. If non-zero then m_userInput must
                                   be a single character. */
} OpalStatusUserInput;


/**Message Waiting information for the OpalIndMessageWaiting indication.
   This is only returned from the OpalGetMessage() function.
  */
typedef struct OpalStatusMessageWaiting {
  const char * m_party;     ///< Party for which the MWI is directed
  const char * m_type;      ///< Type for MWI
  const char * m_extraInfo; ///< Extra information for the MWI
} OpalStatusMessageWaiting;


/**Call clearance information for the OpalIndCallCleared indication.
   This is only returned from the OpalGetMessage() function.
  */
typedef struct OpalStatusCallCleared {
  const char * m_callToken;   ///< Call token for call being cleared.
  const char * m_reason;      ///< String representing the reason for teh call completing.
} OpalStatusCallCleared;


/** Message to/from OPAL system.
    This is passed via the OpalGetMessage() or OpalSendMessage() functions.
  */
struct OpalMessage {
  OpalMessageType m_type;   ///< Type of message
  union {
    const char *             m_commandError;       ///< Used by OpalIndCommandError
    OpalParamGeneral         m_general;            ///< Used by OpalCmdSetGeneralParameters
    OpalParamProtocol        m_protocol;           ///< Used by OpalCmdSetProtocolParameters
    OpalParamRegistration    m_registrationInfo;   ///< Used by OpalCmdRegistration
    OpalStatusRegistration   m_registrationStatus; ///< Used by OpalIndRegistrationStatus
    OpalParamSetUpCall       m_callSetUp;          ///< Used by OpalCmdSetUpCall
    const char *             m_callToken;          ///< Used by OpalCmdAnswerCall/OpalCmdRefuseCall/OpalCmdClearCall/OpalIndAlerting/OpalIndEstablished
    OpalStatusIncomingCall   m_incomingCall;       ///< Used by OpalIndIncomingCall
    OpalStatusMediaStream    m_mediaStream;        ///< Used by OpalIndMediaStream
    OpalStatusUserInput      m_userInput;          ///< Used by OpalIndUserInput
    OpalStatusMessageWaiting m_messageWaiting;     ///< Used by OpalIndMessageWaiting
    OpalStatusCallCleared    m_callCleared;        ///< Used by OpalIndCallCleared
  } m_param;
};


#ifdef __cplusplus
};
#endif

#endif // __OPAL_H


/////////////////////////////////////////////////////////////////////////////
