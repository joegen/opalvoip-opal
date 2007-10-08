/*
 * opalpluginmgr.h
 *
 * OPAL codec plugins handler
 *
 * OPAL Library
 *
 * Copyright (C) 2005-2006 Post Increment
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
 * $Log: opalpluginmgr.h,v $
 * Revision 1.2022.2.2  2007/10/08 02:49:21  rjongbloed
 * Update fixes from HEAD
 *
 * Revision 2.22  2007/10/08 01:45:16  rjongbloed
 * Fixed bad virtual function causing uninitialised variable whcih prevented video from working.
 * Some more clean ups.
 *
 * Revision 2.21.2.1  2007/10/06 04:00:17  rjongbloed
 * First cut at new Media Options negotiation
 *
 * Revision 2.21  2007/10/05 04:14:47  rjongbloed
 * Quite a large code clean up.
 *
 * Revision 2.20  2007/09/25 09:49:54  rjongbloed
 * Fixed videoFastUpdate, is not a count but a simple boolean.
 *
 * Revision 2.19  2007/09/19 10:43:00  csoutheren
 * Exposed G.7231 capability class
 * Added macros to create empty transcoders and capabilities
 *
 * Revision 2.18  2007/09/12 04:19:53  rjongbloed
 * CHanges to avoid creation of long duration OpalMediaFormat instances, eg in
 *   the plug in capabilities, that then do not get updated values from the master
 *   list, or worse from the user modified master list, causing much confusion.
 *
 * Revision 2.17  2007/09/10 03:15:04  rjongbloed
 * Fixed issues in creating and subsequently using correctly unique
 *   payload types in OpalMediaFormat instances and transcoders.
 *
 * Revision 2.16  2007/09/07 17:49:47  ykiryanov
 * File was missing transcoders.h include. This had an effect while building code with no video.
 *
 * Revision 2.15  2007/08/13 06:07:51  csoutheren
 * Expose more functions
 *
 * Revision 2.14  2007/08/08 17:35:15  csoutheren
 * Final plugin manager changes
 *
 * Revision 2.13  2007/08/08 11:18:47  csoutheren
 * Fixed Linux compile errors
 *
 * Revision 2.12  2007/08/08 08:59:06  csoutheren
 * More plugin manager changes, as the last approach dead-ended :(
 *
 * Revision 2.11  2007/08/08 07:12:38  csoutheren
 * #ifdef out unused code - to be removed later if nobody complains :)
 *
 * Revision 2.10  2007/08/07 09:04:02  csoutheren
 * Export more functions
 *
 * Revision 2.9  2007/08/07 08:25:16  csoutheren
 * Expose plugin media format classes
 *
 * Revision 2.8  2007/08/06 07:14:22  csoutheren
 * Fix logging
 * Correct matching of H.263 capabilities
 *
 * Revision 2.7  2007/08/03 08:04:56  csoutheren
 * Add PrintOn for plugin video caps
 *
 * Revision 2.6  2007/07/02 18:53:36  csoutheren
 * Exposed classes
 *
 * Revision 2.5  2007/06/22 05:41:47  rjongbloed
 * Major codec API update:
 *   Automatically map OpalMediaOptions to SIP/SDP FMTP parameters.
 *   Automatically map OpalMediaOptions to H.245 Generic Capability parameters.
 *   Largely removed need to distinguish between SIP and H.323 codecs.
 *   New mechanism for setting OpalMediaOptions from within a plug in.
 *
 * Revision 2.4  2006/10/02 13:30:50  rjongbloed
 * Added LID plug ins
 *
 * Revision 2.3  2006/09/28 07:42:14  csoutheren
 * Merge of useful SRTP implementation
 *
 * Revision 2.2  2006/08/11 07:52:01  csoutheren
 * Fix problem with media format factory in VC 2005
 * Fixing problems with Speex codec
 * Remove non-portable usages of PFactory code
 *
 * Revision 2.1  2006/07/24 14:03:38  csoutheren
 * Merged in audio and video plugins from CVS branch PluginBranch
 *
 * Revision 1.1.2.3  2006/04/06 01:21:16  csoutheren
 * More implementation of video codec plugins
 *
 * Revision 1.1.2.2  2006/03/23 07:55:18  csoutheren
 * Audio plugin H.323 capability merging completed.
 * GSM, LBC, G.711 working. Speex and LPC-10 are not
 *
 * Revision 1.1.2.1  2006/03/16 07:06:00  csoutheren
 * Initial support for audio plugins
 *
 * Created from OpenH323 h323pluginmgr.h
 * Revision 1.24  2005/11/30 13:05:01  csoutheren
 */

#ifndef __OPALPLUGINMGR_H
#define __OPALPLUGINMGR_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/object.h>

#include <opal/buildopts.h>

#include <ptlib/pluginmgr.h>
#include <ptlib/pfactory.h>
#include <codec/opalplugin.h>
#include <opal/mediafmt.h>
#include <opal/transcoders.h>

#if OPAL_H323
#include <h323/h323caps.h>
#endif

#if OPAL_VIDEO
#include <codec/vidcodec.h>
#endif


///////////////////////////////////////////////////////////////////////////////

class H323Capability;

class H323StaticPluginCodec
{
  public:
    virtual ~H323StaticPluginCodec() { }
    virtual PluginCodec_GetAPIVersionFunction Get_GetAPIFn() = 0;
    virtual PluginCodec_GetCodecFunction Get_GetCodecFn() = 0;
};


typedef PFactory<H323StaticPluginCodec> H323StaticPluginCodecFactory;


///////////////////////////////////////////////////////////////////////////////

class OpalPluginCodecManager;

class OpalPluginCodecHandler : public PObject
{
  PCLASSINFO(OpalPluginCodecHandler, PObject);
  public:
    OpalPluginCodecHandler();

#if OPAL_AUDIO
    virtual OpalMediaFormat * OnCreateAudioFormat(OpalPluginCodecManager & mgr,
                                            const PluginCodec_Definition * encoderCodec,
                                                              const char * rtpEncodingName,
                                                                  unsigned frameTime,
                                                                  unsigned timeUnits,
                                                                    time_t timeStamp);
#endif

#if OPAL_VIDEO
    virtual OpalMediaFormat * OnCreateVideoFormat(OpalPluginCodecManager & mgr,
                                            const PluginCodec_Definition * encoderCodec,
                                                              const char * rtpEncodingName,
                                                                    time_t timeStamp);
    virtual void RegisterVideoTranscoder(const PString & src, const PString & dst, PluginCodec_Definition * codec, BOOL v);
#endif

#if OPAL_T38FAX
    virtual OpalMediaFormat * OnCreateFaxFormat(OpalPluginCodecManager & mgr,
                                          const PluginCodec_Definition * encoderCodec,
                                                            const char * rtpEncodingName,
                                                                unsigned frameTime,
                                                                unsigned timeUnits,
                                                                  time_t timeStamp);
#endif
};


class OpalPluginCodecManager : public PPluginModuleManager
{
  PCLASSINFO(OpalPluginCodecManager, PPluginModuleManager);
  public:
    OpalPluginCodecManager(PPluginManager * pluginMgr = NULL);
    ~OpalPluginCodecManager();

    void RegisterStaticCodec(const H323StaticPluginCodecFactory::Key_T & name,
                             PluginCodec_GetAPIVersionFunction getApiVerFn,
                             PluginCodec_GetCodecFunction getCodecFn);

    void OnLoadPlugin(PDynaLink & dll, INT code);

    virtual void OnShutdown();

    static void Bootstrap();

#if OPAL_H323
    H323Capability * CreateCapability(
          const PString & _mediaFormat, 
          const PString & _baseName,
                 unsigned maxFramesPerPacket, 
                 unsigned recommendedFramesPerPacket,
                 unsigned _pluginSubType);
#endif

  protected:
    void RegisterPluginPair(
      PluginCodec_Definition * _encoderCodec,
      PluginCodec_Definition * _decoderCodec,
      OpalPluginCodecHandler * handler
    );

    // Note the below MUST NOT be an OpalMediaFormatList
    PList<OpalMediaFormat> mediaFormatsOnHeap;

    void RegisterCodecPlugins  (unsigned int count, PluginCodec_Definition * codecList, OpalPluginCodecHandler * handler);
    void UnregisterCodecPlugins(unsigned int count, PluginCodec_Definition * codecList, OpalPluginCodecHandler * handler);

#if OPAL_H323
    void RegisterCapability(PluginCodec_Definition * encoderCodec, PluginCodec_Definition * decoderCodec);
    struct CapabilityListCreateEntry {
      CapabilityListCreateEntry(PluginCodec_Definition * e, PluginCodec_Definition * d)
        : encoderCodec(e), decoderCodec(d) { }
      PluginCodec_Definition * encoderCodec;
      PluginCodec_Definition * decoderCodec;
    };
    typedef vector<CapabilityListCreateEntry> CapabilityCreateListType;
    CapabilityCreateListType capabilityCreateList;
#endif
};


///////////////////////////////////////////////////////////////////////////////

class OpalPluginControl
{
  public:
    OpalPluginControl(const PluginCodec_Definition * def, const char * name);

    BOOL Exists() const
    {
      return controlDef != NULL;
    }

    int Call(void * parm, unsigned * parmLen, void * context = NULL) const
    {
      return controlDef != NULL ? (*controlDef->control)(codecDef, context, fnName, parm, parmLen) : 0;
    }

    int Call(void * parm, unsigned   parmLen, void * context = NULL) const
    {
      return Call(parm, &parmLen, context);
    }

    const char * GetName() const { return fnName; }

  protected:
    const PluginCodec_Definition  * codecDef;
    const char                    * fnName;
    const PluginCodec_ControlDefn * controlDef;
};


///////////////////////////////////////////////////////////////////////////////

class OpalPluginMediaFormat
{
  public:
    OpalPluginMediaFormat(const PluginCodec_Definition * defn);

    void PopulateOptions(OpalMediaFormat & format);
    bool IsValidForProtocol(const PString & _protocol) const;
    BOOL AdjustOptions(OpalMediaFormat & fmt, OpalPluginControl & control) const;

    const PluginCodec_Definition * codecDef;
    OpalPluginControl getOptionsControl;
    OpalPluginControl freeOptionsControl;
    OpalPluginControl validForProtocolControl;
    OpalPluginControl toNormalisedControl;
    OpalPluginControl toCustomisedControl;
};


class OpalPluginTranscoder
{
  public:
    OpalPluginTranscoder(const PluginCodec_Definition * defn, BOOL isEnc);
    ~OpalPluginTranscoder();

    BOOL Transcode(const void * from, unsigned * fromLen, void * to, unsigned * toLen, unsigned * flags) const
    {
      return codecDef != NULL && codecDef->codecFunction != NULL &&
            (codecDef->codecFunction)(codecDef, context, from, fromLen, to, toLen, flags) != 0;
    }

  protected:
    const PluginCodec_Definition * codecDef;
    BOOL   isEncoder;
    void * context;

    OpalPluginControl setCodecOptions;
    OpalPluginControl getOutputDataSizeControl;
};


///////////////////////////////////////////////////////////////////////////////

#if OPAL_AUDIO

class OpalPluginAudioMediaFormat : public OpalAudioFormat, public OpalPluginMediaFormat
{
  public:
    friend class OpalPluginCodecManager;

    OpalPluginAudioMediaFormat(
      const PluginCodec_Definition * _encoderCodec,
      const char * rtpEncodingName, /// rtp encoding name
      unsigned frameTime,           /// Time for frame in RTP units (if applicable)
      unsigned /*timeUnits*/,       /// RTP units for frameTime (if applicable)
                         time_t timeStamp              /// timestamp (for versioning)
    );
    bool IsValidForProtocol(const PString & protocol) const;
    PObject * Clone() const;
    virtual BOOL ToNormalisedOptions();
    virtual BOOL ToCustomisedOptions();
};


class OpalPluginFramedAudioTranscoder : public OpalFramedTranscoder, public OpalPluginTranscoder
{
  PCLASSINFO(OpalPluginFramedAudioTranscoder, OpalFramedTranscoder);
  public:
    OpalPluginFramedAudioTranscoder(PluginCodec_Definition * _codec, BOOL _isEncoder, const char * rawFormat = OpalPCM16);
    BOOL ConvertFrame(const BYTE * input, PINDEX & consumed, BYTE * output, PINDEX & created);
    virtual BOOL ConvertSilentFrame(BYTE * buffer);
};


class OpalPluginStreamedAudioTranscoder : public OpalStreamedTranscoder, public OpalPluginTranscoder
{
  PCLASSINFO(OpalPluginStreamedAudioTranscoder, OpalStreamedTranscoder);
  public:
    OpalPluginStreamedAudioTranscoder(PluginCodec_Definition * _codec, BOOL _isEncoder, unsigned inputBits, unsigned outputBits, PINDEX optimalBits);
};


class OpalPluginStreamedAudioEncoder : public OpalPluginStreamedAudioTranscoder
{
  PCLASSINFO(OpalPluginStreamedAudioEncoder, OpalPluginStreamedAudioTranscoder);
  public:
    OpalPluginStreamedAudioEncoder(PluginCodec_Definition * _codec, BOOL);
    int ConvertOne(int _sample) const;
};

class OpalPluginStreamedAudioDecoder : public OpalPluginStreamedAudioTranscoder
{
  PCLASSINFO(OpalPluginStreamedAudioDecoder, OpalPluginStreamedAudioTranscoder);
  public:
    OpalPluginStreamedAudioDecoder(PluginCodec_Definition * _codec, BOOL);
    int ConvertOne(int codedSample) const;
};

#endif

///////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

class OpalPluginVideoMediaFormat : public OpalVideoFormat, public OpalPluginMediaFormat
{
  public:
    friend class OpalPluginCodecManager;
    OpalPluginVideoMediaFormat(
      const PluginCodec_Definition * _encoderCodec,
      const char * rtpEncodingName, /// rtp encoding name
      time_t timeStamp              /// timestamp (for versioning)
    );
    PObject * Clone() const;
    bool IsValidForProtocol(const PString & protocol) const;
    virtual BOOL ToNormalisedOptions();
    virtual BOOL ToCustomisedOptions();
};


class OpalPluginVideoTranscoder : public OpalVideoTranscoder, public OpalPluginTranscoder
{
  PCLASSINFO(OpalPluginVideoTranscoder, OpalVideoTranscoder);
  public:
    OpalPluginVideoTranscoder(const PluginCodec_Definition * _codec, BOOL _isEncoder);
    ~OpalPluginVideoTranscoder();

    PINDEX GetOptimalDataFrameSize(BOOL input) const;
    BOOL ConvertFrames(const RTP_DataFrame & src, RTP_DataFrameList & dstList);
    BOOL UpdateOutputMediaFormat(const OpalMediaFormat & fmt);

  protected:
    RTP_DataFrame * bufferRTP;
};

#endif

///////////////////////////////////////////////////////////////////////////////

#if OPAL_T38FAX

class OpalPluginFaxMediaFormat : public OpalMediaFormat, public OpalPluginMediaFormat
{
  public:
    friend class OpalPluginCodecManager;

    OpalPluginFaxMediaFormat(
      const PluginCodec_Definition * _encoderCodec,
      const char * rtpEncodingName, /// rtp encoding name
      unsigned frameTime,
      unsigned /*timeUnits*/,           /// RTP units for frameTime (if applicable)
      time_t timeStamp              /// timestamp (for versioning)
    );
    PObject * Clone() const;
    bool IsValidForProtocol(const PString & protocol) const;
};

#endif // OPAL_T38FAX


//////////////////////////////////////////////////////
//
//  this is the base class for codecs accesible via the abstract factory functions
//

/**Class for codcs which is accessible via the abstract factory functions.
   The code would be :

      PFactory<OpalFactoryCodec>::CreateInstance(conversion);

  to create an instance, where conversion is (eg) "L16:G.711-uLaw-64k"
*/
class OpalFactoryCodec : public PObject {
  PCLASSINFO(OpalFactoryCodec, PObject)
  public:
  /** Return the PluginCodec_Definition, which describes this codec */
    virtual const struct PluginCodec_Definition * GetDefinition()
    { return NULL; }

  /** Return the sourceFormat field of PluginCodec_Definition for this codec*/
    virtual PString GetInputFormat() const = 0;

  /** Return the destFormat field of PluginCodec_Definition for this codec*/
    virtual PString GetOutputFormat() const = 0;

    /** Take the supplied data and apply the conversion specified by CreateInstance call (above). When this method returns, toLen contains the number of bytes placed in the destination buffer. */
    virtual int Encode(const void * from,      ///< pointer to the source data
                         unsigned * fromLen,   ///< number of bytes in the source data to process
                             void * to,        ///< pointer to the destination buffer, which contains the output of the  conversion
		                 unsigned * toLen,     ///< Number of available bytes in the destination buffer
                     unsigned int * flag       ///< Typically, this is not used.
		       ) = 0;  

    /** Return the  sampleRate field of PluginCodec_Definition for this codec*/
    virtual unsigned int GetSampleRate() const = 0;

    /** Return the  bitsPerSec field of PluginCodec_Definition for this codec*/
    virtual unsigned int GetBitsPerSec() const = 0;

    /** Return the  nmPerFrame field of PluginCodec_Definition for this codec*/
    virtual unsigned int GetFrameTime() const = 0;

    /** Return the samplesPerFrame  field of PluginCodec_Definition for this codec*/
    virtual unsigned int GetSamplesPerFrame() const = 0;

    /** Return the  bytesPerFrame field of PluginCodec_Definition for this codec*/
    virtual unsigned int GetBytesPerFrame() const = 0;

    /** Return the recommendedFramesPerPacket field of PluginCodec_Definition for this codec*/
    virtual unsigned int GetRecommendedFramesPerPacket() const = 0;

    /** Return the maxFramesPerPacket field of PluginCodec_Definition for this codec*/
    virtual unsigned int GetMaxFramesPerPacket() const = 0;

    /** Return the  rtpPayload field of PluginCodec_Definition for this codec*/
    virtual BYTE         GetRTPPayload() const = 0;

    /** Return the  sampleRate field of PluginCodec_Definition for this codec*/
    virtual PString      GetSDPFormat() const = 0;
};

//////////////////////////////////////////////////////////////////////////////

template<class TranscoderClass>
class OpalPluginTranscoderFactory : public OpalTranscoderFactory
{
  public:
    class Worker : public OpalTranscoderFactory::WorkerBase 
    {
      public:
        Worker(const OpalTranscoderKey & key, PluginCodec_Definition * _codecDefn, BOOL _isEncoder)
          : OpalTranscoderFactory::WorkerBase(), codecDefn(_codecDefn), isEncoder(_isEncoder)
        { OpalTranscoderFactory::Register(key, this); }

      protected:
        virtual OpalTranscoder * Create(const OpalTranscoderKey &) const
        { return new TranscoderClass(codecDefn, isEncoder); }

        PluginCodec_Definition * codecDefn;
        BOOL isEncoder;
    };
};

//////////////////////////////////////////////////////////////////////////////
//
// Helper class for handling plugin capabilities
//

class H323PluginCapabilityInfo
{
  public:
    H323PluginCapabilityInfo(const PluginCodec_Definition * _encoderCodec,
                             const PluginCodec_Definition * _decoderCodec);

    H323PluginCapabilityInfo(const PString & _baseName);

    const PString & GetFormatName() const
    { return capabilityFormatName; }

  protected:
    const PluginCodec_Definition * encoderCodec;
    const PluginCodec_Definition * decoderCodec;
    PString                        capabilityFormatName;
};


#if OPAL_AUDIO

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling most audio plugin capabilities
//

class H323AudioPluginCapability : public H323AudioCapability,
                                  public H323PluginCapabilityInfo
{
  PCLASSINFO(H323AudioPluginCapability, H323AudioCapability);
  public:
    H323AudioPluginCapability(const PluginCodec_Definition * _encoderCodec,
                              const PluginCodec_Definition * _decoderCodec,
                              unsigned _pluginSubType);

    // this constructor is only used when creating a capability without a codec
    H323AudioPluginCapability(const PString & _mediaFormat,
                              const PString & _baseName,
                              unsigned _pluginSubType);

    virtual PObject * Clone() const;

    virtual PString GetFormatName() const;

    virtual unsigned GetSubType() const;

  protected:
    unsigned pluginSubType;
    //unsigned h323subType;   // only set if using capability without codec
};

#define OPAL_DECLARE_EMPTY_AUDIO_CAPABILITY(fmt, type) \
class fmt##_CapabilityRegisterer { \
  public: \
    fmt##_CapabilityRegisterer() \
    { H323CapabilityFactory::Register(fmt, new H323AudioPluginCapability(fmt, fmt, type)); } \
}; \

#define OPAL_DEFINE_EMPTY_AUDIO_CAPABILITY(fmt) \
static fmt##_CapabilityRegisterer fmt##_CapabilityRegisterer_instance; \

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling G.723.1 codecs
//

class H323PluginG7231Capability : public H323AudioPluginCapability
{
  PCLASSINFO(H323PluginG7231Capability, H323AudioPluginCapability);
  public:
    H323PluginG7231Capability(const PluginCodec_Definition * _encoderCodec,
                              const PluginCodec_Definition * _decoderCodec,
                              BOOL _annexA = TRUE);

    // this constructor is used for creating empty codecs
    H323PluginG7231Capability(const OpalMediaFormat & fmt, BOOL _annexA = TRUE);

    virtual PObject * Clone() const;
    virtual BOOL OnSendingPDU(H245_AudioCapability & cap, unsigned packetSize) const;
    virtual BOOL OnReceivedPDU(const H245_AudioCapability & cap,  unsigned & packetSize);

  protected:
    BOOL annexA;
};

#define OPAL_DECLARE_EMPTY_G7231_CAPABILITY(fmt, annex) \
class fmt##_CapabilityRegisterer { \
  public: \
    fmt##_CapabilityRegisterer() \
    { H323CapabilityFactory::Register(fmt, new H323PluginG7231Capability(fmt, annex)); } \
}; \

#define OPAL_DEFINE_EMPTY_G7231_CAPABILITY(fmt) \
static fmt##_CapabilityRegisterer fmt##_CapabilityRegisterer_instance; \

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling non standard audio capabilities
//

class H323CodecPluginNonStandardAudioCapability : public H323NonStandardAudioCapability,
                                                  public H323PluginCapabilityInfo
{
  PCLASSINFO(H323CodecPluginNonStandardAudioCapability, H323NonStandardAudioCapability);
  public:
    H323CodecPluginNonStandardAudioCapability(const PluginCodec_Definition * _encoderCodec,
                                              const PluginCodec_Definition * _decoderCodec,
                                              H323NonStandardCapabilityInfo::CompareFuncType compareFunc,
                                              const unsigned char * data, unsigned dataLen);

    H323CodecPluginNonStandardAudioCapability(const PluginCodec_Definition * _encoderCodec,
                                              const PluginCodec_Definition * _decoderCodec,
                                              const unsigned char * data, unsigned dataLen);

    virtual PObject * Clone() const;

    virtual PString GetFormatName() const;
};

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling generic audio capabilities
//

class H323CodecPluginGenericAudioCapability : public H323GenericAudioCapability,
                                              public H323PluginCapabilityInfo
{
  PCLASSINFO(H323CodecPluginGenericAudioCapability, H323GenericAudioCapability);
  public:
    H323CodecPluginGenericAudioCapability(const PluginCodec_Definition * _encoderCodec,
                                          const PluginCodec_Definition * _decoderCodec,
				          const PluginCodec_H323GenericCodecData * data);

    virtual PObject * Clone() const;
    virtual PString GetFormatName() const;
};

#endif

#if OPAL_VIDEO

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling most audio plugin capabilities
//

class H323VideoPluginCapability : public H323VideoCapability,
                             public H323PluginCapabilityInfo
{
  PCLASSINFO(H323VideoPluginCapability, H323VideoCapability);
  public:
    H323VideoPluginCapability(const PluginCodec_Definition * _encoderCodec,
                              const PluginCodec_Definition * _decoderCodec,
                              unsigned _pluginSubType);

    virtual PString GetFormatName() const;

    virtual unsigned GetSubType() const;

    static BOOL SetCommonOptions(OpalMediaFormat & mediaFormat, int frameWidth, int frameHeight, int frameRate);

    virtual void PrintOn(std::ostream & strm) const;

  protected:
    unsigned pluginSubType;
    unsigned h323subType;   // only set if using capability without codec
};

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling non standard video capabilities
//

class H323CodecPluginNonStandardVideoCapability : public H323NonStandardVideoCapability,
                                                  public H323PluginCapabilityInfo
{
  PCLASSINFO(H323CodecPluginNonStandardVideoCapability, H323NonStandardVideoCapability);
  public:
    H323CodecPluginNonStandardVideoCapability(const PluginCodec_Definition * _encoderCodec,
                                              const PluginCodec_Definition * _decoderCodec,
                                              H323NonStandardCapabilityInfo::CompareFuncType compareFunc,
                                              const unsigned char * data, unsigned dataLen);

    H323CodecPluginNonStandardVideoCapability(const PluginCodec_Definition * _encoderCodec,
                                              const PluginCodec_Definition * _decoderCodec,
                                              const unsigned char * data, unsigned dataLen);

    virtual PObject * Clone() const;

    virtual PString GetFormatName() const;
};

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling generic video capabilities
//

class H323CodecPluginGenericVideoCapability : public H323GenericVideoCapability,
                                              public H323PluginCapabilityInfo
{
  PCLASSINFO(H323CodecPluginGenericVideoCapability, H323GenericVideoCapability);
  public:
    H323CodecPluginGenericVideoCapability(const PluginCodec_Definition * _encoderCodec,
                                          const PluginCodec_Definition * _decoderCodec,
				          const PluginCodec_H323GenericCodecData * data);

    virtual PObject * Clone() const;

    virtual PString GetFormatName() const;
};

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling H.261 plugin capabilities
//

class H323H261PluginCapability : public H323VideoPluginCapability
{
  PCLASSINFO(H323H261PluginCapability, H323VideoPluginCapability);
  public:
    H323H261PluginCapability(const PluginCodec_Definition * _encoderCodec,
                             const PluginCodec_Definition * _decoderCodec);

    Comparison Compare(const PObject & obj) const;

    virtual PObject * Clone() const;

    virtual BOOL OnSendingPDU(
      H245_VideoCapability & pdu  /// PDU to set information on
    ) const;

    virtual BOOL OnSendingPDU(
      H245_VideoMode & pdu
    ) const;

    virtual BOOL OnReceivedPDU(
      const H245_VideoCapability & pdu  /// PDU to get information from
    );
};

//////////////////////////////////////////////////////////////////////////////
//
// Class for handling H.263 plugin capabilities
//

class H323H263PluginCapability : public H323VideoPluginCapability
{
  PCLASSINFO(H323H263PluginCapability, H323VideoPluginCapability);
  public:
    H323H263PluginCapability(const PluginCodec_Definition * _encoderCodec,
                             const PluginCodec_Definition * _decoderCodec);

    Comparison Compare(const PObject & obj) const;

    virtual PObject * Clone() const;

    virtual BOOL OnSendingPDU(
      H245_VideoCapability & pdu  /// PDU to set information on
    ) const;

    virtual BOOL OnSendingPDU(
      H245_VideoMode & pdu
    ) const;

    virtual BOOL OnReceivedPDU(
      const H245_VideoCapability & pdu  /// PDU to get information from
    );
    virtual BOOL IsMatch(const PASN_Choice & subTypePDU) const;
};

#endif // OPAL_VIDEO

#endif // __OPALPLUGINMGR_H
