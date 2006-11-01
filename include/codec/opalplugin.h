/*
 * opalplugins.h
 *
 * OPAL codec plugins handler
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (C) 2004-2006 Post Increment
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
 * $Log: opalplugin.h,v $
 * Revision 1.2007  2006/11/01 06:57:23  csoutheren
 * Fixed usage of YUV frame header
 *
 * Revision 2.5  2006/10/17 13:04:36  shorne
 * comments were accidently truncated
 *
 * Revision 2.4  2006/10/17 04:09:08  shorne
 * Corrected generic parameter identifiers & added generic codec identifiers
 *
 * Revision 2.3  2006/10/02 13:30:50  rjongbloed
 * Added LID plug ins
 *
 * Revision 2.2  2006/08/11 07:52:00  csoutheren
 * Fix problem with media format factory in VC 2005
 * Fixing problems with Speex codec
 * Remove non-portable usages of PFactory code
 *
 * Revision 2.1  2006/07/24 14:03:38  csoutheren
 * Merged in audio and video plugins from CVS branch PluginBranch
 *
 * Revision 1.1.2.4  2006/04/24 09:09:37  csoutheren
 * Added LGPL license
 *
 * Revision 1.1.2.3  2006/04/19 07:52:30  csoutheren
 * Add ability to have SIP-only and H.323-only codecs, and implement for H.261
 *
 * Revision 1.1.2.2  2006/04/06 01:21:16  csoutheren
 * More implementation of video codec plugins
 *
 * Revision 1.1.2.1  2006/03/16 07:06:00  csoutheren
 * Initial support for audio plugins
 *
 * Revision 1.9  2005/11/21 21:04:10  shorne
 * Added more HID input switches
 *
 * Revision 1.8  2005/08/23 08:13:06  shorne
 * Added HID plugin volume & LCD display support
 *
 * Revision 1.7  2005/07/03 13:54:23  shorne
 * Added Initial LID Plugin Support
 *
 * Revision 1.6  2005/06/07 03:22:22  csoutheren
 * Added patch 1198741 with support for plugin codecs with generic capabilities
 * Added patch 1198754 with support for setting quality level on audio codecs
 * Added patch 1198760 with GSM-AMR codec support
 * Many thanks to Richard van der Hoff for his work
 *
 * Revision 1.5  2004/12/20 23:30:20  csoutheren
 * Added plugin support for packet loss concealment frames
 *
 * Revision 1.4  2004/11/29 06:30:53  csoutheren
 * Added support for wideband codecs
 *
 * Revision 1.3  2004/05/18 22:26:28  csoutheren
 * Initial support for embedded codecs
 * Fixed problems with streamed codec support
 * Updates for abstract factory loading methods
 *
 * Revision 1.2  2004/05/09 14:44:36  csoutheren
 * Added support for streamed plugin audio codecs
 *
 * Revision 1.1  2004/04/09 12:25:25  csoutheren
 * Renamed from h323plugin.h
 *
 * Revision 1.2  2004/04/03 10:38:24  csoutheren
 * Added in initial cut at codec plugin code. Branches are for wimps :)
 *
 * Revision 1.1.2.1  2004/03/31 11:03:16  csoutheren
 * Initial public version
 *
 * Revision 1.8  2004/02/23 13:17:32  craigs
 * Fixed problems with codec interface functions
 *
 * Revision 1.7  2004/02/23 13:04:09  craigs
 * Removed warnings when compliing plugins
 *
 * Revision 1.6  2004/01/27 14:55:46  craigs
 * Implemented static linking of new codecs
 *
 * Revision 1.5  2004/01/23 05:21:15  craigs
 * Updated for changes to the codec plugin interface
 *
 * Revision 1.4  2004/01/09 11:27:46  craigs
 * Plugin codec audio now works :)
 *
 * Revision 1.3  2004/01/09 07:32:22  craigs
 * More fixes for capability problems
 *
 * Revision 1.2  2004/01/06 07:05:03  craigs
 * Changed to support plugin codecs
 *
 * Revision 1.1  2004/01/04 13:37:51  craigs
 * Implementation of codec plugins
 *
 *
 */

#ifndef __OPAL_CODECPLUGIN_H
#define __OPAL_CODECPLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

#ifdef _WIN32
#  ifdef PLUGIN_CODEC_DLL_EXPORTS
#    define PLUGIN_CODEC_DLL_API __declspec(dllexport)
#  else
#    define PLUGIN_CODEC_DLL_API __declspec(dllimport)
#  endif

#else

#define PLUGIN_CODEC_DLL_API

#endif

#define PWLIB_PLUGIN_API_VERSION 0

#define	PLUGIN_CODEC_VERSION		         1    // initial version
#define	PLUGIN_CODEC_VERSION_WIDEBAND		 2    // added wideband
#define	PLUGIN_CODEC_VERSION_VIDEO		     3    // added video

#define PLUGIN_CODEC_API_VER_FN       PWLibPlugin_GetAPIVersion
#define PLUGIN_CODEC_API_VER_FN_STR   "PWLibPlugin_GetAPIVersion"

#define PLUGIN_CODEC_GET_CODEC_FN     OpalCodecPlugin_GetCodecs
#define PLUGIN_CODEC_GET_CODEC_FN_STR "OpalCodecPlugin_GetCodecs"

#define PLUGIN_CODEC_API_VER_FN_DECLARE \
PLUGIN_CODEC_DLL_API unsigned int PLUGIN_CODEC_API_VER_FN() \
{ return PWLIB_PLUGIN_API_VERSION; }

enum {
  PluginCodec_License_None                           = 0,
  PluginCodec_Licence_None = PluginCodec_License_None,        // allow for old code with misspelled constant
  PluginCodec_License_GPL                            = 1,
  PluginCodec_License_MPL                            = 2,
  PluginCodec_License_Freeware                       = 3,
  PluginCodec_License_ResearchAndDevelopmentUseOnly  = 4,
  PluginCodec_License_BSD                            = 5,
  PluginCodec_License_LGPL                           = 6,

  PluginCodec_License_NoRoyalties                    = 0x7f,

  // any license codes above here require royalty payments
  PluginCodec_License_RoyaltiesRequired              = 0x80
};

struct PluginCodec_information {
  // start of version 1 fields
  time_t timestamp;                     // codec creation time and date - obtain with command: date -u "+%c = %s"

  const char * sourceAuthor;            // source code author
  const char * sourceVersion;           // source code version
  const char * sourceEmail;             // source code email contact information
  const char * sourceURL;               // source code web site
  const char * sourceCopyright;         // source code copyright
  const char * sourceLicense;           // source code license
  unsigned char sourceLicenseCode;      // source code license

  const char * codecDescription;        // codec description
  const char * codecAuthor;             // codec author
  const char * codecVersion;            // codec version
  const char * codecEmail;              // codec email contact information
  const char * codecURL;                // codec web site
  const char * codecCopyright;          // codec copyright information
  const char * codecLicense;            // codec license
  unsigned short codecLicenseCode;      // codec license code
  // end of version 1 fields

};

enum PluginCodec_Flags {
  PluginCodec_MediaTypeMask          = 0x000f,
  PluginCodec_MediaTypeAudio         = 0x0000,
  PluginCodec_MediaTypeVideo         = 0x0001,
  PluginCodec_MediaTypeAudioStreamed = 0x0002,

  PluginCodec_InputTypeMask          = 0x0010,
  PluginCodec_InputTypeRaw           = 0x0000,
  PluginCodec_InputTypeRTP           = 0x0010,

  PluginCodec_OutputTypeMask         = 0x0020,
  PluginCodec_OutputTypeRaw          = 0x0000,
  PluginCodec_OutputTypeRTP          = 0x0020,

  PluginCodec_RTPTypeMask            = 0x0040,
  PluginCodec_RTPTypeDynamic         = 0x0000,
  PluginCodec_RTPTypeExplicit        = 0x0040,

  PluginCodec_RTPSharedMask          = 0x0080,
  PluginCodec_RTPTypeNotShared       = 0x0000,
  PluginCodec_RTPTypeShared          = 0x0080,

  PluginCodec_DecodeSilenceMask      = 0x0100,
  PluginCodec_NoDecodeSilence        = 0x0000,
  PluginCodec_DecodeSilence          = 0x0100,

  PluginCodec_EncodeSilenceMask      = 0x0200,
  PluginCodec_NoEncodeSilence        = 0x0000,
  PluginCodec_EncodeSilence          = 0x0200,

  PluginCodec_BitsPerSamplePos       = 12,
  PluginCodec_BitsPerSampleMask      = 0xf000,
};

enum PluginCodec_CoderFlags {
  PluginCodec_CoderSilenceFrame      = 1,    // request audio codec to create silence frame
  PluginCodec_CoderForceIFrame       = 2     // request video codec to force I frame
};

enum PluginCodec_ReturnCoderFlags {
  PluginCodec_ReturnCoderLastFrame     = 1,    // indicates when video codec returns last data for frame
  PluginCodec_ReturnCoderIFrame        = 2,    // indicates when video returns I frame
  PluginCodec_ReturnCoderRequestIFrame = 4     // indicates when video decoder request I frame for resync
};

struct PluginCodec_Definition;

struct PluginCodec_ControlDefn {
  const char * name;
  int (*control)(const struct PluginCodec_Definition * codec, void * context, 
                 const char * name, void * parm, unsigned * parmLen);
 
};

struct PluginCodec_Definition {
  unsigned int version;			               // codec structure version

  // start of version 1 fields
  struct PluginCodec_information * info;   // license information

  unsigned int flags;                      // b0-3: 0 = audio,        1 = video
                                           // b4:   0 = raw input,    1 = RTP input
                                           // b5:   0 = raw output,   1 = RTP output
                                           // b6:   0 = dynamic RTP,  1 = explicit RTP
                                           // b7:   0 = no share RTP, 1 = share RTP

  const char * descr;    		               // text decription

  const char * sourceFormat;               // source format
  const char * destFormat;                 // destination format

  const void * userData;                   // user data value

  unsigned int sampleRate;                 // samples per second
  unsigned int bitsPerSec;     		       // raw bits per second

  unsigned int nsPerFrame;                 // nanoseconds per frame
  unsigned int samplesPerFrame;		       // audio: samples per frame,                      video: frame width
  unsigned int bytesPerFrame;              // audio: max bytes per frame,                    video: frame height
  unsigned int recommendedFramesPerPacket; // audio: recommended number of frames per packet video: recommended frame rate
  unsigned int maxFramesPerPacket;         // audio: maximum number of frames per packet     video: max frame rate

  unsigned char rtpPayload;    		         // IANA RTP payload code (if defined)
  const char * sdpFormat;                  // SDP format string (or NULL, if no SDP format)

  void * (*createCodec)(const struct PluginCodec_Definition * codec);	                  // create codec 
  void (*destroyCodec) (const struct PluginCodec_Definition * codec,  void * context); 	// destroy codec
  int (*codecFunction) (const struct PluginCodec_Definition * codec,  void * context,   // do codec function
                                  const void * from, unsigned * fromLen,
                                        void * to,   unsigned * toLen,
                                        unsigned int * flag);
  struct PluginCodec_ControlDefn * codecControls;
 
  // H323 specific fields
  unsigned char h323CapabilityType;
  void          * h323CapabilityData;

  // end of version 1 fields
};

typedef struct PluginCodec_Definition * (* PluginCodec_GetCodecFunction)(unsigned int *, unsigned int);
typedef unsigned (* PluginCodec_GetAPIVersionFunction)();

///////////////////////////////////////////////////////////////////
//
//  H.323 specific values
//


struct PluginCodec_H323CapabilityExtension {
  unsigned int index;
  void * data;
  unsigned dataLength;
};

struct PluginCodec_H323NonStandardCodecData {
  const char * objectId;
  unsigned char  t35CountryCode;
  unsigned char  t35Extension;
  unsigned short manufacturerCode;
  const unsigned char * data;
  unsigned int dataLength;
  int (*capabilityMatchFunction)(struct PluginCodec_H323NonStandardCodecData *);
};


struct PluginCodec_H323GenericParameterDefinition
{
    int collapsing; /* boolean */
    unsigned int id;
    enum PluginCodec_H323GenericParameterType {
	/* these need to be in the same order as the choices in
	   H245_ParameterValue::Choices, as the value is just cast to that type
	*/
	PluginCodec_GenericParameter_Logical = 0,
	PluginCodec_GenericParameter_BooleanArray,
	PluginCodec_GenericParameter_unsignedMin,
	PluginCodec_GenericParameter_unsignedMax,
	PluginCodec_GenericParameter_unsigned32Min,
	PluginCodec_GenericParameter_unsigned32Max,
	PluginCodec_GenericParameter_OctetString,
	PluginCodec_GenericParameter_GenericParameter
    } type;
    union {
	unsigned long integer;
	char *octetstring;
	struct PluginCodec_H323GenericParameterDefinition *genericparameter;
    } value;
};

    
struct PluginCodec_H323GenericCodecData {
    // XXX need a way of specifying non-standard identifiers?
    
    // some cunning structures & lists, and associated logic in 
    // H323CodecPluginGenericAudioCapability::H323CodecPluginGenericAudioCapability()
    const char * standardIdentifier;
    unsigned int maxBitRate;

    /* parameters; these are the parameters which are set in the
       'TerminalCapabilitySet' and 'OpenLogicalChannel' requests */
    unsigned int nParameters;
    /* an array of nParameters parameter definitions */
    const struct PluginCodec_H323GenericParameterDefinition *params; 
};
    
struct PluginCodec_H323AudioGSMData {
  int comfortNoise:1;
  int scrambled:1;
};

struct  PluginCodec_H323AudioG7231AnnexC {
  unsigned char maxAl_sduAudioFrames;
  int silenceSuppression:1;
  int highRateMode0:6;  	      // INTEGER (27..78),	-- units octets
  int	highRateMode1:6;	        // INTEGER (27..78),	-- units octets
  int	lowRateMode0:6;		        // INTEGER (23..66),	-- units octets
  int	lowRateMode1:6;		        // INTEGER (23..66),	-- units octets
  int	sidMode0:4;		            // INTEGER (6..17),	-- units octets
  int	sidMode1:4;		            // INTEGER (6..17),	-- units octets
};

/* Following H.261 structure is no longer used */
/*
struct PluginCodec_H323VideoH261
{
  unsigned char qcifMPI;                           //	INTEGER (1..4) OPTIONAL,	-- units 1/29.97 Hz (0 if not present)
  unsigned char cifMPI;		                         // INTEGER (1..4) OPTIONAL,	-- units 1/29.97 Hz (0 if not present)
	unsigned char temporalSpatialTradeOffCapability; //	BOOLEAN,
  unsigned char stillImageTransmission;            //	BOOLEAN,	-- Annex D of H.261
	int maxBitRate;                                  //	INTEGER (1..19200),	-- units of 100 bit/s
  unsigned char videoBadMBsCap;                    //	BOOLEAN
};
*/

/* Following H.263 structure is no longer used */
/*
struct PluginCodec_H323VideoH263
{
	unsigned char sqcifMPI;                          // INTEGER (1..32) OPTIONAL, -- units 1/29.97 Hz (0 if not present)
	unsigned char qcifMPI;                           // INTEGER (1..32) OPTIONAL, -- units 1/29.97 Hz (0 if not present)
	unsigned char cifMPI;                            // INTEGER (1..32) OPTIONAL, -- units 1/29.97 Hz (0 if not present)
	unsigned char cif4MPI;                           // INTEGER (1..32) OPTIONAL, -- units 1/29.97 Hz (0 if not present)
	unsigned char cif16MPI;                          // INTEGER (1..32) OPTIONAL, -- units 1/29.97 Hz (0 if not present)
	unsigned int maxBitRate;                         // INTEGER (1..192400) units 100 bit/s
	unsigned char unrestrictedVector;                //	BOOLEAN
	unsigned char arithmeticCoding;                  //	BOOLEAN,
	unsigned char advancedPrediction;                //	BOOLEAN,
	unsigned char pbFrames;                          //	BOOLEAN,
	unsigned char temporalSpatialTradeOffCapability; //	BOOLEAN,
	int hrdB;                                        // INTEGER (0..524287) OPTIONAL, -- units 128 bits (-1 if not present)
	int bppMaxKb;                                    // INTEGER (0..65535) OPTIONAL, -- units 1024 bits (-1 if not present)

	unsigned int slowSqcifMPI;                       // INTEGER (1..3600) OPTIONAL, --  units seconds/frame (0 if not present)
	unsigned int slowQcifMPI;                        //	INTEGER (1..3600) OPTIONAL, -- units seconds/frame (0 if not present)
	unsigned int slowCifMPI;                         //	INTEGER (1..3600) OPTIONAL, -- units seconds/frame (0 if not present)
	unsigned int slowCif4MPI;                        //	INTEGER (1..3600) OPTIONAL, -- units seconds/frame (0 if not present)
	unsigned int slowCif16MPI;                       //	INTEGER (1..3600) OPTIONAL, -- units seconds/frame (0 if not present)
	unsigned char errorCompensation;                 // BOOLEAN,

	void * enhancementLayerInfo;                     // not yet supported (NULL if not present)
	void * h263Options;                              // not yet support (NULL if not present)
};
*/

enum {
  PluginCodec_H323Codec_undefined,			// must be zero, so empty struct is undefined
  PluginCodec_H323Codec_programmed,			// H323ProgrammedCapability
  PluginCodec_H323Codec_nonStandard,		// H323NonStandardData
  PluginCodec_H323Codec_generic,            // H323GenericCodecData

  // audio codecs
  PluginCodec_H323AudioCodec_g711Alaw_64k,		    // int
  PluginCodec_H323AudioCodec_g711Alaw_56k,		    // int
  PluginCodec_H323AudioCodec_g711Ulaw_64k,		    // int
  PluginCodec_H323AudioCodec_g711Ulaw_56k,		    // int
  PluginCodec_H323AudioCodec_g722_64k,			      // int
  PluginCodec_H323AudioCodec_g722_56k,			      // int
  PluginCodec_H323AudioCodec_g722_48k,			      // int
  PluginCodec_H323AudioCodec_g7231,			          // H323AudioG7231Data
  PluginCodec_H323AudioCodec_g728,			          // int
  PluginCodec_H323AudioCodec_g729,			          // int
  PluginCodec_H323AudioCodec_g729AnnexA,		      // int
  PluginCodec_H323AudioCodec_is11172,             // not yet implemented
  PluginCodec_H323AudioCodec_is13818Audio,        // not yet implemented
  PluginCodec_H323AudioCodec_g729wAnnexB,		      // int
  PluginCodec_H323AudioCodec_g729AnnexAwAnnexB,	  // int
  PluginCodec_H323AudioCodec_g7231AnnexC,         // H323AudioG7231AnnexC
  PluginCodec_H323AudioCodec_gsmFullRate,		      // H323AudioGSMData
  PluginCodec_H323AudioCodec_gsmHalfRate,		      // H323AudioGSMData
  PluginCodec_H323AudioCodec_gsmEnhancedFullRate,	// H323AudioGSMData
  PluginCodec_H323AudioCodec_g729Extensions,      // not yet implemented

  // video codecs
  PluginCodec_H323VideoCodec_h261,                // implemented 
  PluginCodec_H323VideoCodec_h262,                // not yet implemented
  PluginCodec_H323VideoCodec_h263,                // implemented
  PluginCodec_H323VideoCodec_is11172,             // not yet implemented

  // special codes
  PluginCodec_H323Codec_NoH323 = 0xff,            // used for SIP-only codecs
};

/////////////////
//
// Generic Codec Standard Identifiers
//

// Audio Capabilities
// AMR (as defined in H.245v13 Annex I)
#define OpalPluginCodec_Identifer_AMR             "0.0.8.245.1.1.1"

// AMR-NB\WB  (as defined in H.245v13 Annex R)
#define OpalPluginCodec_Identifer_AMR_NB          "0.0.8.1.1.9"
#define OpalPluginCodec_Identifer_AMR_WB          "0.0.8.1.1.10"

// iLBC (as defined in H.245v13 Annex S)
#define OpalPluginCodec_Identifer_iLBC            "0.0.8.1.1.11"


// Video Capabilities

// H264 (as defined in H.241)
#define OpalPluginCodec_Identifer_H264            "0.0.8.241.0.0.0.0"
#define OpalPluginCodec_Identifer_H264_RFC3984    "0.0.8.241.0.0.0.1"
#define OpalPluginCodec_Identifer_H264_RFC3984Int "0.0.8.241.0.0.0.2"
#define OpalPluginCodec_Identifer_H264_Generic    "0.0.8.241.0.0.1"

// ISO/IEC 14496-2 MPEG4 part 2 (as defined in H.245v13 Annex E)
#define OpalPluginCodec_Identifer_MPEG4           "0.0.8.245.1.0.0"


/////////////////
//
// video specific definitions
//

struct PluginCodec_Video_FrameHeader {
  unsigned int  x;
  unsigned int  y;
  unsigned int  width;
  unsigned int  height;
};

#ifdef __cplusplus
};

inline unsigned char * OPAL_VIDEO_FRAME_DATA_PTR(struct PluginCodec_Video_FrameHeader * base)
{ return (((unsigned char *)base) + sizeof(PluginCodec_Video_FrameHeader)); }

inline unsigned char * OPAL_VIDEO_FRAME_DATA_PTR(const PluginCodec_Video_FrameHeader * base)
{ return (((unsigned char *)base) + sizeof(PluginCodec_Video_FrameHeader)); }

extern "C" {
#endif

#define PLUGIN_CODEC_VIDEO_SET_FRAME_SIZE_FN    "set_frame_size"    // argument is struct PluginCodec_VideoSetFrameInfo
struct PluginCodec_Video_SetFrameInfo {
  int width;
  int height;
};


/////////////////
//
// experimental definitions for statically linking codecs
//

#ifdef OPAL_STATIC_CODEC

#  undef PLUGIN_CODEC_DLL_API
#  define PLUGIN_CODEC_DLL_API static
#  define PLUGIN_CODEC_IMPLEMENT(name) \
unsigned int Opal_StaticCodec_##name##_GetAPIVersion() \
{ return PWLIB_PLUGIN_API_VERSION; } \
static struct PluginCodec_Definition * PLUGIN_CODEC_GET_CODEC_FN(unsigned * count, unsigned /*version*/); \
struct PluginCodec_Definition * Opal_StaticCodec_##name##_GetCodecs(unsigned * p1, unsigned p2) \
{ return PLUGIN_CODEC_GET_CODEC_FN(p1,p2); } \

#else

#  define PLUGIN_CODEC_IMPLEMENT(name) \
PLUGIN_CODEC_DLL_API unsigned int PLUGIN_CODEC_API_VER_FN() \
{ return PWLIB_PLUGIN_API_VERSION; } \

#endif

#ifdef __cplusplus
};
#endif


#endif // __OPAL_CODECPLUGIN_H
